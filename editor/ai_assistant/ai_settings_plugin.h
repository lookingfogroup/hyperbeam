/**************************************************************************/
/*  ai_settings_plugin.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             HYPERBEAM ENGINE                           */
/*                        https://github.com/lookingfogroup/hyperbeam     */
/**************************************************************************/

#pragma once

#include "editor/settings/editor_settings.h"
#include "scene/gui/control.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/label.h"
#include "scene/gui/vbox_container.h"
#include "scene/gui/grid_container.h"

class AISettingsInterface : public Control {
	GDCLASS(AISettingsInterface, Control)

private:
	VBoxContainer *main_container = nullptr;
	GridContainer *settings_grid = nullptr;
	
	// AI Provider Settings
	OptionButton *provider_selection = nullptr;
	LineEdit *api_key_input = nullptr;
	LineEdit *api_endpoint_input = nullptr;
	OptionButton *model_selection = nullptr;
	
	// AI Behavior Settings
	SpinBox *temperature_setting = nullptr;
	SpinBox *max_tokens_setting = nullptr;
	CheckBox *auto_complete_enabled = nullptr;
	CheckBox *code_suggestions_enabled = nullptr;
	CheckBox *chat_memory_enabled = nullptr;
	
	// UI Settings
	CheckBox *dock_visible = nullptr;
	OptionButton *dock_position = nullptr;
	CheckBox *show_ai_notifications = nullptr;
	
	void _setup_ui();
	void _load_settings();
	void _save_settings();
	void _on_provider_changed(int p_index);
	void _on_setting_changed();
	
protected:
	static void _bind_methods();
	
public:
	void refresh_settings();
	void reset_to_defaults();
	
	AISettingsInterface();
};
