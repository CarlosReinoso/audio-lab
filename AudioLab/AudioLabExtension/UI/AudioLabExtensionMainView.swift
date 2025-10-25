//
//  AudioLabExtensionMainView.swift
//  AudioLabExtension
//
//  Created by Veronica Crespo on 25/10/2025.
//

import SwiftUI

struct AudioLabExtensionMainView: View {
    var parameterTree: ObservableAUParameterGroup
    
    // Easy to change version number
    private let version = "v1.2"
    
    private let octaveOptions = [
        ("2↑", 0, "2 Octaves Up"),
        ("1↑", 1, "1 Octave Up"),
        ("N", 2, "Normal"),
        ("1↓", 3, "1 Octave Down"),
        ("2↓", 4, "2 Octaves Down")
    ]
    
    var body: some View {
        ZStack {
            VStack(spacing: 25) {
                Text("Guitar Octave Shifter")
                    .font(.title2)
                    .fontWeight(.bold)
                
                Text("Select Octave")
                    .font(.headline)
                    .foregroundColor(.secondary)
                
                // Octave buttons in a grid
                LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 8), count: 3), spacing: 12) {
                    ForEach(octaveOptions, id: \.1) { option in
                        Button(action: {
                            parameterTree.global.octaveShift.value = AUValue(option.1)
                        }) {
                            VStack(spacing: 4) {
                                Text(option.0)
                                    .font(.title2)
                                    .fontWeight(.bold)
                                Text(option.2)
                                    .font(.caption2)
                                    .multilineTextAlignment(.center)
                            }
                            .frame(maxWidth: .infinity)
                            .frame(height: 60)
                            .background(
                                RoundedRectangle(cornerRadius: 12)
                                    .fill(parameterTree.global.octaveShift.value == AUValue(option.1) ? 
                                          Color.accentColor : Color.secondary.opacity(0.2))
                            )
                            .foregroundColor(parameterTree.global.octaveShift.value == AUValue(option.1) ? 
                                           .white : .primary)
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
                .padding(.horizontal, 20)
            }
            .padding()
            
            // Version number in bottom left corner
            VStack {
                Spacer()
                HStack {
                    Text(version)
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .padding(.leading, 16)
                        .padding(.bottom, 8)
                    Spacer()
                }
            }
        }
    }
}
