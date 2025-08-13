/**************************************************************************/
/*  ai_assistant_plugin.h                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             HYPERBEAM ENGINE                           */
/*                        https://github.com/lookingfogroup/hyperbeam     */
/**************************************************************************/
/* Copyright (c) 2025-present Hyperbeam Engine contributors.             */
/* Based on Godot Engine (c) 2014-present Godot Engine contributors.     */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "editor/plugins/editor_plugin.h"
#include "scene/gui/control.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/split_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/panel_container.h"
#include "core/io/http_client.h"
#include "core/io/json.h"

class AICodeCompletionProvider;
class AIChatInterface;
class AIContextAnalyzer;

class AIAssistantPlugin : public EditorPlugin {
	GDCLASS(AIAssistantPlugin, EditorPlugin)

private:
	static AIAssistantPlugin *singleton;
	
	// Main UI Components
	Control *ai_dock = nullptr;
	VBoxContainer *main_container = nullptr;
	
	// Chat Interface
	AIChatInterface *chat_interface = nullptr;
	RichTextLabel *chat_display = nullptr;
	LineEdit *chat_input = nullptr;
	Button *send_button = nullptr;
	ScrollContainer *chat_scroll = nullptr;
	
	// Code Completion
	AICodeCompletionProvider *completion_provider = nullptr;
	
	// Context Analysis
	AIContextAnalyzer *context_analyzer = nullptr;
	
	// Settings
	String api_key;
	String api_endpoint = "https://api.anthropic.com/v1/messages";
	String model_name = "claude-3-5-sonnet-20241022";
	
	// State
	bool is_enabled = false;
	PackedStringArray chat_history;
	
	void _notification(int p_what);
	void _setup_ui();
	void _setup_chat_interface();
	void _setup_code_completion();
	void _on_chat_send_pressed();
	void _on_chat_input_text_submitted(const String &p_text);
	void _send_ai_request(const String &p_message);
	void _handle_ai_response(const String &p_response);
	void _add_chat_message(const String &p_sender, const String &p_message);
	
	// API Integration
	void _make_api_request(const String &p_prompt, const String &p_context = "");
	String _build_system_prompt();
	String _get_current_editor_context();
	
protected:
	static void _bind_methods();

public:
	static AIAssistantPlugin *get_singleton();
	
	virtual String get_plugin_name() const override { return "AI Assistant"; }
	virtual void _enable_plugin() override;
	virtual void _disable_plugin() override;
	
	// Code Completion API
	void provide_code_completion(const String &p_code, int p_cursor_pos, Array &r_completions);
	void explain_code_selection(const String &p_code);
	void suggest_code_improvements(const String &p_code);
	
	// Chat API
	void ask_ai_question(const String &p_question);
	void get_help_with_error(const String &p_error_message, const String &p_context);
	
	// Settings
	void set_api_key(const String &p_key);
	String get_api_key() const { return api_key; }
	void set_model_name(const String &p_model);
	String get_model_name() const { return model_name; }
	
	AIAssistantPlugin();
	~AIAssistantPlugin();
};

// AI Code Completion Provider
class AICodeCompletionProvider : public RefCounted {
	GDCLASS(AICodeCompletionProvider, RefCounted)

private:
	AIAssistantPlugin *plugin = nullptr;
	
public:
	void set_plugin(AIAssistantPlugin *p_plugin) { plugin = p_plugin; }
	void request_completion(const String &p_code, int p_cursor_pos);
	void analyze_code_context(const String &p_code);
	
	AICodeCompletionProvider() = default;
};

// AI Chat Interface
class AIChatInterface : public Control {
	GDCLASS(AIChatInterface, Control)

private:
	AIAssistantPlugin *plugin = nullptr;
	VBoxContainer *message_container = nullptr;
	LineEdit *input_field = nullptr;
	Button *send_button = nullptr;
	
	void _on_send_pressed();
	void _on_input_submitted(const String &p_text);
	
protected:
	static void _bind_methods();
	
public:
	void set_plugin(AIAssistantPlugin *p_plugin) { plugin = p_plugin; }
	void add_message(const String &p_sender, const String &p_content);
	void clear_chat();
	
	AIChatInterface();
};

// AI Context Analyzer
class AIContextAnalyzer : public RefCounted {
	GDCLASS(AIContextAnalyzer, RefCounted)

private:
	struct ProjectContext {
		String project_name;
		PackedStringArray scene_files;
		PackedStringArray script_files;
		String current_scene;
		String current_script;
		Dictionary project_settings;
	};
	
	ProjectContext current_context;
	
public:
	void analyze_current_project();
	String get_context_summary();
	String get_relevant_documentation(const String &p_topic);
	PackedStringArray get_available_nodes();
	PackedStringArray get_available_methods(const String &p_class_name);
	
	AIContextAnalyzer() = default;
};
