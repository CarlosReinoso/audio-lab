//
//  AudioLabExtensionDSPKernel.hpp
//  AudioLabExtension
//
//  Created by Veronica Crespo on 25/10/2025.
//

#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <algorithm>
#import <vector>
#import <span>

#import "AudioLabExtensionParameterAddresses.h"

/*
 AudioLabExtensionDSPKernel
 As a non-ObjC class, this is safe to use from render thread.
 */
class AudioLabExtensionDSPKernel {
public:
    void initialize(int inputChannelCount, int outputChannelCount, double inSampleRate) {
        mSampleRate = inSampleRate;
        mInputChannelCount = inputChannelCount;
        mOutputChannelCount = outputChannelCount;
        
        // Initialize granular synthesis buffers with improved settings
        mBufferSize = (int)(mSampleRate * 2.0); // 2 second buffer for better quality
        mGrainSize = (int)(mSampleRate * 0.05); // 50ms grain size for smoother transitions
        mOverlap = 8; // 8x overlap for very smooth sound
        
        mDelayBuffers.resize(inputChannelCount);
        mWritePositions.resize(inputChannelCount);
        mGrainPositions.resize(inputChannelCount);
        mGrainCounters.resize(inputChannelCount);
        mOutputBuffers.resize(inputChannelCount);
        
        for (int channel = 0; channel < inputChannelCount; ++channel) {
            mDelayBuffers[channel].resize(mBufferSize, 0.0f);
            mOutputBuffers[channel].resize(mGrainSize, 0.0f);
            mWritePositions[channel] = 0;
            mGrainPositions[channel] = 0.0f;
            mGrainCounters[channel] = 0;
        }
        
        // Calculate pitch shift ratios for all octave options
        mPitchRatios[0] = 4.0;      // 2 octaves up (read at 4x speed)
        mPitchRatios[1] = 2.0;      // 1 octave up (read at 2x speed)
        mPitchRatios[2] = 1.0;      // Normal (no shift)
        mPitchRatios[3] = 0.5;      // 1 octave down (read at half speed)
        mPitchRatios[4] = 0.25;     // 2 octaves down (read at quarter speed)
        
        // Initialize window function (Hann window)
        mWindowSize = mGrainSize / mOverlap;
        mWindow.resize(mWindowSize);
        for (int i = 0; i < mWindowSize; ++i) {
            mWindow[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (mWindowSize - 1)));
        }
    }
    
    void deInitialize() {
        mDelayBuffers.clear();
        mWritePositions.clear();
        mGrainPositions.clear();
        mGrainCounters.clear();
        mOutputBuffers.clear();
        mWindow.clear();
    }
    
    // MARK: - Bypass
    bool isBypassed() {
        return mBypassed;
    }
    
    void setBypass(bool shouldBypass) {
        mBypassed = shouldBypass;
    }
    
    // MARK: - Parameter Getter / Setter
    void setParameter(AUParameterAddress address, AUValue value) {
        switch (address) {
            case AudioLabExtensionParameterAddress::octaveShift:
                mOctaveShift = (int)value;
                break;
        }
    }
    
    AUValue getParameter(AUParameterAddress address) {
        // Return the goal. It is not thread safe to return the ramping value.
        
        switch (address) {
            case AudioLabExtensionParameterAddress::octaveShift:
                return (AUValue)mOctaveShift;
            default: 
                return 0.f;
        }
    }
    
    // MARK: - Max Frames
    AUAudioFrameCount maximumFramesToRender() const {
        return mMaxFramesToRender;
    }
    
    void setMaximumFramesToRender(const AUAudioFrameCount &maxFrames) {
        mMaxFramesToRender = maxFrames;
    }
    
    // MARK: - Musical Context
    void setMusicalContextBlock(AUHostMusicalContextBlock contextBlock) {
        mMusicalContextBlock = contextBlock;
    }
    
    /**
     MARK: - Internal Process
     
     This function does the core signal processing.
     Do your custom DSP here.
     */
    void process(std::span<float const*> inputBuffers, std::span<float *> outputBuffers, AUEventSampleTime bufferStartTime, AUAudioFrameCount frameCount) {
        assert(inputBuffers.size() == outputBuffers.size());
        
        if (mBypassed) {
            // Pass the samples through
            for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
                std::copy_n(inputBuffers[channel], frameCount, outputBuffers[channel]);
            }
            return;
        }
        
        // Process each channel
        for (UInt32 channel = 0; channel < inputBuffers.size(); ++channel) {
            processChannel(inputBuffers[channel], outputBuffers[channel], frameCount, channel);
        }
    }
    
    void handleOneEvent(AUEventSampleTime now, AURenderEvent const *event) {
        switch (event->head.eventType) {
            case AURenderEventParameter: {
                handleParameterEvent(now, event->parameter);
                break;
            }
                
            default:
                break;
        }
    }
    
private:
    void processChannel(const float* input, float* output, AUAudioFrameCount frameCount, int channel) {
        if (mOctaveShift == 2) {
            // Normal pitch (no shifting)
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                output[frameIndex] = input[frameIndex];
            }
        } else {
            // Apply improved granular synthesis pitch shifting
            float pitchRatio = mPitchRatios[mOctaveShift];
            
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                // Write input to delay buffer with anti-aliasing
                float filteredInput = input[frameIndex];
                mDelayBuffers[channel][mWritePositions[channel]] = filteredInput;
                mWritePositions[channel] = (mWritePositions[channel] + 1) % mBufferSize;
                
                // Check if we need to generate a new grain
                if (mGrainCounters[channel] <= 0) {
                    generateGrain(channel, pitchRatio);
                    mGrainCounters[channel] = mWindowSize;
                }
                
                // Output from current grain with improved windowing
                int grainIndex = mWindowSize - mGrainCounters[channel];
                float windowedSample = mOutputBuffers[channel][grainIndex] * mWindow[grainIndex];
                
                // Apply gentle compression to reduce artifacts
                float compressedSample = tanh(windowedSample * 0.8f) * 1.2f;
                output[frameIndex] = compressedSample;
                
                mGrainCounters[channel]--;
            }
        }
    }
    
    void generateGrain(int channel, float pitchRatio) {
        // Calculate read position for this grain with better positioning
        int readStart = (mWritePositions[channel] - mGrainSize + mBufferSize) % mBufferSize;
        
        // Generate grain with improved pitch shifting
        for (int i = 0; i < mGrainSize; ++i) {
            float readPos = readStart + i * pitchRatio;
            
            // Handle wrap-around more carefully
            while (readPos >= mBufferSize) readPos -= mBufferSize;
            while (readPos < 0) readPos += mBufferSize;
            
            int readIndex = (int)readPos;
            int readIndex2 = (readIndex + 1) % mBufferSize;
            float fraction = readPos - readIndex;
            
            // Cubic interpolation for smoother sound
            int readIndex0 = (readIndex - 1 + mBufferSize) % mBufferSize;
            int readIndex3 = (readIndex + 2) % mBufferSize;
            
            float y0 = mDelayBuffers[channel][readIndex0];
            float y1 = mDelayBuffers[channel][readIndex];
            float y2 = mDelayBuffers[channel][readIndex2];
            float y3 = mDelayBuffers[channel][readIndex3];
            
            // Cubic interpolation
            float a = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
            float b = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            float c = -0.5f * y0 + 0.5f * y2;
            float d = y1;
            
            float interpolatedSample = a * fraction * fraction * fraction + 
                                     b * fraction * fraction + 
                                     c * fraction + d;
            
            mOutputBuffers[channel][i] = interpolatedSample;
        }
    }
    
    void handleParameterEvent(AUEventSampleTime now, AUParameterEvent const& parameterEvent) {
        setParameter(parameterEvent.parameterAddress, parameterEvent.value);
    }
    
    // MARK: Member Variables
    AUHostMusicalContextBlock mMusicalContextBlock;
    
    double mSampleRate = 44100.0;
    int mOctaveShift = 2; // Default to "Normal" (index 2)
    bool mBypassed = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;
    
    // Improved granular synthesis variables
    int mInputChannelCount = 2;
    int mOutputChannelCount = 2;
    int mBufferSize = 0;
    int mGrainSize = 0;
    int mOverlap = 8; // Increased overlap for smoother sound
    int mWindowSize = 0;
    
    std::vector<std::vector<float>> mDelayBuffers;
    std::vector<std::vector<float>> mOutputBuffers;
    std::vector<int> mWritePositions;
    std::vector<float> mGrainPositions;
    std::vector<int> mGrainCounters;
    std::vector<float> mWindow;
    float mPitchRatios[5]; // 0: 2 octaves up, 1: 1 octave up, 2: normal, 3: 1 octave down, 4: 2 octaves down
};
