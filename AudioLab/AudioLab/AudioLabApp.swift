//
//  AudioLabApp.swift
//  AudioLab
//
//  Created by Veronica Crespo on 25/10/2025.
//

import SwiftUI

@main
struct AudioLabApp: App {
    private let hostModel = AudioUnitHostModel()

    var body: some Scene {
        WindowGroup {
            ContentView(hostModel: hostModel)
        }
    }
}
