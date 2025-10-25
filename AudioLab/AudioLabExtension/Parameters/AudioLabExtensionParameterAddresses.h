//
//  AudioLabExtensionParameterAddresses.h
//  AudioLabExtension
//
//  Created by Veronica Crespo on 25/10/2025.
//

#pragma once

#include <AudioToolbox/AUParameters.h>

typedef NS_ENUM(AUParameterAddress, AudioLabExtensionParameterAddress) {
    gain = 0,
    octaveShift = 1
};
