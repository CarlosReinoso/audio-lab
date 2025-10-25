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
        
        // Simple delay buffer for pitch shifting
        mBufferSize = (int)(mSampleRate * 0.5); // 0.5 second buffer
        mDelayBuffers.resize(inputChannelCount);
        mWritePositions.resize(inputChannelCount);
        mReadPositions.resize(inputChannelCount);
        
        for (int channel = 0; channel < inputChannelCount; ++channel) {
            mDelayBuffers[channel].resize(mBufferSize, 0.0f);
            mWritePositions[channel] = 0;
            mReadPositions[channel] = 0.0f;
        }
        
        // Calculate pitch shift ratios for all octave options
        mPitchRatios[0] = 4.0;      // 2 octaves up (read at 4x speed)
        mPitchRatios[1] = 2.0;      // 1 octave up (read at 2x speed)
        mPitchRatios[2] = 1.0;      // Normal (no shift)
        mPitchRatios[3] = 0.5;      // 1 octave down (read at half speed)
        mPitchRatios[4] = 0.25;     // 2 octaves down (read at quarter speed)
    }
    
    void deInitialize() {
        mDelayBuffers.clear();
        mWritePositions.clear();
        mReadPositions.clear();
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
            case AudioLabExtensionParameterAddress::gain:
                mGain = value;
                break;
            case AudioLabExtensionParameterAddress::octaveShift:
                mOctaveShift = (int)value;
                break;
        }
    }
    
    AUValue getParameter(AUParameterAddress address) {
        // Return the goal. It is not thread safe to return the ramping value.
        
        switch (address) {
            case AudioLabExtensionParameterAddress::gain:
                return (AUValue)mGain;
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
        // Safety check
        if (channel >= mDelayBuffers.size() || mBufferSize == 0) {
            // Fallback: pass through with gain
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                output[frameIndex] = input[frameIndex] * mGain;
            }
            return;
        }
        
        if (mOctaveShift == 2) {
            // Normal pitch (no shifting)
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                output[frameIndex] = input[frameIndex] * mGain;
            }
        } else {
            // Apply simple pitch shifting
            if (mOctaveShift < 0 || mOctaveShift >= 5) {
                // Invalid octave shift, fallback to normal
                for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                    output[frameIndex] = input[frameIndex] * mGain;
                }
                return;
            }
            
            float pitchRatio = mPitchRatios[mOctaveShift];
            
            for (UInt32 frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
                // Write input to delay buffer
                mDelayBuffers[channel][mWritePositions[channel]] = input[frameIndex];
                mWritePositions[channel] = (mWritePositions[channel] + 1) % mBufferSize;
                
                // Read from delay buffer with pitch shifting
                float readPosition = mReadPositions[channel];
                int readIndex = (int)readPosition;
                float fraction = readPosition - readIndex;
                
                // Ensure valid indices
                if (readIndex < 0) readIndex += mBufferSize;
                if (readIndex >= mBufferSize) readIndex -= mBufferSize;
                
                int readIndex2 = (readIndex + 1) % mBufferSize;
                
                // Linear interpolation
                float sample1 = mDelayBuffers[channel][readIndex];
                float sample2 = mDelayBuffers[channel][readIndex2];
                float interpolatedSample = sample1 + fraction * (sample2 - sample1);
                
                // Apply gain and output
                output[frameIndex] = interpolatedSample * mGain;
                
                // Update read position
                mReadPositions[channel] += pitchRatio;
                if (mReadPositions[channel] >= mBufferSize) {
                    mReadPositions[channel] -= mBufferSize;
                }
                if (mReadPositions[channel] < 0) {
                    mReadPositions[channel] += mBufferSize;
                }
            }
        }
    }
    
    
    void handleParameterEvent(AUEventSampleTime now, AUParameterEvent const& parameterEvent) {
        setParameter(parameterEvent.parameterAddress, parameterEvent.value);
    }
    
    // MARK: Member Variables
    AUHostMusicalContextBlock mMusicalContextBlock;
    
    double mSampleRate = 44100.0;
    double mGain = 1.0;
    int mOctaveShift = 2; // Default to "Normal" (index 2)
    bool mBypassed = false;
    AUAudioFrameCount mMaxFramesToRender = 1024;
    
    // Simple pitch shifting variables
    int mInputChannelCount = 2;
    int mOutputChannelCount = 2;
    int mBufferSize = 0;
    
    std::vector<std::vector<float>> mDelayBuffers;
    std::vector<int> mWritePositions;
    std::vector<float> mReadPositions;
    float mPitchRatios[5]; // 0: 2 octaves up, 1: 1 octave up, 2: normal, 3: 1 octave down, 4: 2 octaves down
};
