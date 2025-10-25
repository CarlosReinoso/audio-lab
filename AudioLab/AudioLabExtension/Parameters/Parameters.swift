//
//  Parameters.swift
//  AudioLabExtension
//
//  Created by Veronica Crespo on 25/10/2025.
//

import Foundation
import AudioToolbox

let AudioLabExtensionParameterSpecs = ParameterTreeSpec {
    ParameterGroupSpec(identifier: "global", name: "Global") {
        ParameterSpec(
            address: .octaveShift,
            identifier: "octaveShift",
            name: "Octave Shift",
            units: .indexed,
            valueRange: 0.0...4.0,
            defaultValue: 2.0,
            valueStrings: ["2 Octaves Up", "1 Octave Up", "Normal", "1 Octave Down", "2 Octaves Down"]
        )
    }
}

extension ParameterSpec {
    init(
        address: AudioLabExtensionParameterAddress,
        identifier: String,
        name: String,
        units: AudioUnitParameterUnit,
        valueRange: ClosedRange<AUValue>,
        defaultValue: AUValue,
        unitName: String? = nil,
        flags: AudioUnitParameterOptions = [AudioUnitParameterOptions.flag_IsWritable, AudioUnitParameterOptions.flag_IsReadable],
        valueStrings: [String]? = nil,
        dependentParameters: [NSNumber]? = nil
    ) {
        self.init(address: address.rawValue,
                  identifier: identifier,
                  name: name,
                  units: units,
                  valueRange: valueRange,
                  defaultValue: defaultValue,
                  unitName: unitName,
                  flags: flags,
                  valueStrings: valueStrings,
                  dependentParameters: dependentParameters)
    }
}
