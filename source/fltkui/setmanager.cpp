/*
 * Nestopia UE
 *
 * Copyright (C) 2012-2024 R. Danbrook
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

#include "setmanager.h"
#include <jg/jg_nes.h>

#include "ini.h"

#include "video.h" // Use UiAdapter for this...

namespace {

jg_setting_t fe_settings[] = {
    { "v_scale", "Initial Window Scale",
      "N = Window scale factor at startup",
      "Set the window's initial scale factor (multiple of NES resolution)",
      2, 1, 16, FLAG_FRONTEND | JG_SETTING_RESTART
    },
    { "v_linearfilter", "Linear Filter",
      "0 = Disable, 1 = Enable",
      "Use the GPU's built-in linear filter for video output",
      1, 0, 1, FLAG_FRONTEND
    },
    { "v_aspect", "Aspect Ratio",
      "0 = TV Correct, 1 = 1:1, 2 = 4:3",
      "Set the aspect ratio to the correct TV aspect, 1:1 (square pixels), or 4:3",
      0, 0, 2, FLAG_FRONTEND
    },
    { "a_rsqual", "Audio Resampler Quality",
      "0 = Sinc (Best), 1 = Sinc (Medium), 2 = Sinc (Fast), 3 = Zero Order Hold, 4 = Linear",
      "Set the frontend's audio resampling quality. Use Sinc unless you are on extremely weak hardware.",
      2, 0, 4, FLAG_FRONTEND
    },
    { "m_ffspeed", "Fast-forward Speed",
      "N = Fast-forward speed multiplier",
      "Set the speed multiplier to run emulation faster",
      2, 2, 8, FLAG_FRONTEND
    },
    { "m_hidecursor", "Hide Cursor",
      "0 = Disabled, 1 = Enabled",
      "Hide the cursor when hovering over the UI",
      0, 0, 1, FLAG_FRONTEND
    },
    { "m_hidecrosshair", "Hide Crosshair",
      "0 = Disabled, 1 = Enabled",
      "Hide the crosshair when a Zapper is present",
      0, 0, 1, FLAG_FRONTEND
    },
};

jg_setting_t nullsetting;

// Input config is really better staying alive in memory in .ini form
mINI::INIStructure inputini;

}

SettingManager::SettingManager() {
    // Set defaults
    size_t numsettings = sizeof(fe_settings) / sizeof(jg_setting_t);
    for (size_t i = 0; i < numsettings; ++i ) {
        settings.push_back(&fe_settings[i]);
    }

    // Create config directory
    if (const char *env_xdg_config = std::getenv("XDG_CONFIG_HOME")) {
        confpath = std::string(env_xdg_config) + "/nestopia";
    }
    else {
        confpath = std::string(std::getenv("HOME")) + "/.config/nestopia";
    }

    // Create the directory if it does not exist
    std::filesystem::create_directories(confpath);
}

void SettingManager::read(JGManager& jgm) {
    // Read in any settings
    mINI::INIFile file(confpath + "/nestopia.conf");
    mINI::INIStructure ini;
    file.read(ini);

    for (const auto& setting : settings) {
        std::string& strval = ini["frontend"][setting->name];

        if (strval.empty()) {
            continue;
        }

        int val = std::stoi(strval);
        if (val >= setting->min && val <= setting->max) {
            setting->val = val;
        }
    }

    for (const auto& setting : *jgm.get_settings()) {
        std::string& strval = ini["nestopia"][setting->name];

        if (strval.empty()) {
            continue;
        }

        int val = std::stoi(strval);
        if (val >= setting->min && val <= setting->max) {
            setting->val = val;
        }
    }

    jgm.rehash();

    // Read input config
    mINI::INIFile inputfile(confpath + "/input.conf");
    inputfile.read(inputini);
}

void SettingManager::write(JGManager& jgm) {
    std::string filepath(confpath + "/nestopia.conf");
    std::ofstream os(filepath);

    if (!os.is_open()) {
        return;
    }

    os << "; Nestopia UE Configuration File\n\n";

    // Write out frontend settings
    os << "[frontend]\n";
    for (const auto& setting : settings) {
        os << "; " << setting->desc << "\n";
        os << "; " << setting->opts << "\n";
        os << setting->name << " = " << setting->val << "\n\n";
    }

    // Write out emulator core settings
    os << "[nestopia]\n";
    for (const auto& setting : *jgm.get_settings()) {
        os << "; " << setting->desc << "\n";
        os << "; " << setting->opts << "\n";
        os << setting->name << " = " << setting->val << "\n\n";
    }

    os.close();

    mINI::INIFile inputfile(confpath + "/input.conf");
    inputfile.write(inputini, true);

    /*for (auto const& it : inputini) {
        auto const& section = it.first;
        auto const& collection = it.second;

        std::cout << "[" << section << "]" << std::endl;

        for (auto const& it2 : collection) {
            auto const& key = it2.first;
            auto const& value = it2.second;
            std::cout << key << " = " << value << std::endl;
        }
    }*/
}

std::vector<jg_setting_t*> *SettingManager::get_settings() {
    return &settings;
}

jg_setting_t* SettingManager::get_setting(std::string name) {
    size_t numsettings = sizeof(fe_settings) / sizeof(jg_setting_t);
    for (size_t i = 0; i < numsettings; ++i ) {
        if (std::string(fe_settings[i].name) == name) {
            return &fe_settings[i];
        }
    }
    return &nullsetting;
}

std::string& SettingManager::get_input(std::string name, std::string def) {
    return inputini[name][def];
}

void SettingManager::set_input(std::string name, std::string def, std::string val) {
    inputini[name][def] = val;
}