# AudioLab - AUv3 Loopy Pro Plugin Octave Shifter

A professional-grade Audio Unit v3 (AUv3) plugin designed specifically for Loopy Pro that provides high-quality octave shifting for guitar and other instruments.

## Features

- **5 Octave Options**: 2 octaves up, 1 octave up, normal, 1 octave down, 2 octaves down
- **Granular Synthesis**: Advanced pitch shifting algorithm for smooth, natural sound
- **Guitar-to-Bass Conversion**: Perfect for transforming guitar into bass-like sounds
- **Real-time Processing**: Low-latency audio processing optimized for live performance
- **Intuitive Interface**: Clean button-based UI for easy octave selection
- **Loopy Pro Compatible**: Designed specifically for use with Loopy Pro

## Technical Specifications

- **Audio Unit Type**: Effect (aufx)
- **Sample Rates**: 44.1kHz, 48kHz, 96kHz
- **Bit Depth**: 32-bit float
- **Channels**: Stereo (2 in, 2 out)
- **Platform**: iOS (AUv3)

## Algorithm

Uses advanced granular synthesis with:
- 50ms grain size for smooth transitions
- 8x overlap for seamless audio
- Cubic interpolation for high-quality pitch shifting
- Hann windowing to prevent artifacts
- Gentle compression to reduce noise

## Installation

1. Build the project in Xcode
2. Install on your iOS device
3. Load in Loopy Pro as an AUv3 effect
4. Select your desired octave shift

## Usage

Perfect for:
- Guitar-to-bass conversion
- Creating bass lines from guitar
- Adding depth to guitar parts
- Live performance octave effects
- Studio recording applications

## Version

Current version: v1.2

## Requirements

- iOS 13.0+
- Loopy Pro or compatible AUv3 host
- Xcode 14.0+ (for building)

## License

This project is based on Apple's AUv3 template and follows Apple's licensing terms.
