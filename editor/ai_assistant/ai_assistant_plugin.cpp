/**************************************************************************/
/*  ai_assistant_plugin.cpp                                               */
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

#include "ai_assistant_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/http_client.h"
#include "core/io/json.h"
#include "editor/editor_interface.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor/editor_settings.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/label.h"

AIAssistantPlugin *AIAssistantPlugin::singleton = nullptr;

AIAssistantPlugin *AIAssistantPlugin::get_singleton() {
	return singleton;
}

void AIAssistantPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_api_key", "key"), &AIAssistantPlugin::set_api_key);
	ClassDB::bind_method(D_METHOD("get_api_key"), &AIAssistantPlugin::get_api_key);
	ClassDB::bind_method(D_METHOD("set_model_name", "model"), &AIAssistantPlugin::set_model_name);
	ClassDB::bind_method(D_METHOD("get_model_name"), &AIAssistantPlugin::get_model_name);
	ClassDB::bind_method(D_METHOD("ask_ai_question", "question"), &AIAssistantPlugin::ask_ai_question);
	ClassDB::bind_method(D_METHOD("explain_code_selection", "code"), &AIAssistantPlugin::explain_code_selection);
	ClassDB::bind_method(D_METHOD("suggest_code_improvements", "code"), &AIAssistantPlugin::suggest_code_improvements);
	
	ClassDB::bind_method(D_METHOD("_on_chat_send_pressed"), &AIAssistantPlugin::_on_chat_send_pressed);
	ClassDB::bind_method(D_METHOD("_on_chat_input_text_submitted", "text"), &AIAssistantPlugin::_on_chat_input_text_submitted);
}

AIAssistantPlugin::AIAssistantPlugin() {
	singleton = this;
	completion_provider = memnew(AICodeCompletionProvider);
	completion_provider->set_plugin(this);
	context_analyzer = memnew(AIContextAnalyzer);
}

AIAssistantPlugin::~AIAssistantPlugin() {
	if (completion_provider) {
		memdelete(completion_provider);
	}
	if (context_analyzer) {
		memdelete(context_analyzer);
	}
	singleton = nullptr;
}

void AIAssistantPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_setup_ui();
			break;
		case NOTIFICATION_EXIT_TREE:
			if (ai_dock) {
				remove_control_from_docks(ai_dock);
			}
			break;
	}
}

void AIAssistantPlugin::_enable_plugin() {
	is_enabled = true;
	_setup_ui();
	
	// Load API key from editor settings
	EditorSettings *settings = EditorSettings::get_singleton();
	if (settings->has_setting("ai_assistant/api_key")) {
		api_key = settings->get_setting("ai_assistant/api_key");
	}
	if (settings->has_setting("ai_assistant/model_name")) {
		model_name = settings->get_setting("ai_assistant/model_name");
	}
	
	print_line("AI Assistant Plugin enabled - Hyperbeam is ready for agentic development!");
}

void AIAssistantPlugin::_disable_plugin() {
	is_enabled = false;
	if (ai_dock) {
		remove_control_from_docks(ai_dock);
		ai_dock = nullptr;
	}
	print_line("AI Assistant Plugin disabled");
}

void AIAssistantPlugin::_setup_ui() {
	if (ai_dock) {
		return; // Already set up
	}
	
	// Create main dock container
	ai_dock = memnew(PanelContainer);
	ai_dock->set_name("AI Assistant");
	
	main_container = memnew(VBoxContainer);
	ai_dock->add_child(main_container);
	
	// Add title
	Label *title = memnew(Label);
	title->set_text("🚀 Hyperbeam AI Assistant");
	title->add_theme_font_size_override("font_size", 16);
	main_container->add_child(title);
	
	// Add separator
	HSeparator *separator = memnew(HSeparator);
	main_container->add_child(separator);
	
	_setup_chat_interface();
	_setup_code_completion();
	
	// Add to dock
	add_control_to_dock(DOCK_SLOT_LEFT_UL, ai_dock);
}

void AIAssistantPlugin::_setup_chat_interface() {
	// Chat display area
	chat_scroll = memnew(ScrollContainer);
	chat_scroll->set_custom_minimum_size(Size2(0, 300));
	main_container->add_child(chat_scroll);
	
	chat_display = memnew(RichTextLabel);
	chat_display->set_bbcode_enabled(true);
	chat_display->set_selection_enabled(true);
	chat_display->set_context_menu_enabled(true);
	chat_display->set_fit_content(true);
	chat_scroll->add_child(chat_display);
	
	// Initial welcome message
	_add_chat_message("Hyperbeam", "Welcome to Hyperbeam AI Assistant! 🎮\n\nI'm here to help with your game development. Ask me about:\n• GDScript and C# coding\n• Game design patterns\n• Godot-specific features\n• Debugging help\n• Performance optimization\n\nType your question below!");
	
	// Input area
	HBoxContainer *input_container = memnew(HBoxContainer);
	main_container->add_child(input_container);
	
	chat_input = memnew(LineEdit);
	chat_input->set_placeholder("Ask me anything about game development...");
	chat_input->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	input_container->add_child(chat_input);
	
	send_button = memnew(Button);
	send_button->set_text("Send");
	input_container->add_child(send_button);
	
	// Connect signals
	chat_input->connect("text_submitted", callable_mp(this, &AIAssistantPlugin::_on_chat_input_text_submitted));
	send_button->connect("pressed", callable_mp(this, &AIAssistantPlugin::_on_chat_send_pressed));
}

void AIAssistantPlugin::_setup_code_completion() {
	// Add code assistance buttons
	Label *code_label = memnew(Label);
	code_label->set_text("Code Assistance:");
	main_container->add_child(code_label);
	
	VBoxContainer *button_container = memnew(VBoxContainer);
	main_container->add_child(button_container);
	
	Button *explain_btn = memnew(Button);
	explain_btn->set_text("Explain Selected Code");
	button_container->add_child(explain_btn);
	
	Button *improve_btn = memnew(Button);
	improve_btn->set_text("Suggest Improvements");
	button_container->add_child(improve_btn);
	
	Button *debug_btn = memnew(Button);
	debug_btn->set_text("Help Debug");
	button_container->add_child(debug_btn);
	
	// Connect code assistance buttons
	explain_btn->connect("pressed", callable_mp(this, &AIAssistantPlugin::explain_code_selection).bind(""));
	improve_btn->connect("pressed", callable_mp(this, &AIAssistantPlugin::suggest_code_improvements).bind(""));
}

void AIAssistantPlugin::_on_chat_send_pressed() {
	String message = chat_input->get_text();
	if (!message.is_empty()) {
		_send_ai_request(message);
		chat_input->clear();
	}
}

void AIAssistantPlugin::_on_chat_input_text_submitted(const String &p_text) {
	if (!p_text.is_empty()) {
		_send_ai_request(p_text);
		chat_input->clear();
	}
}

void AIAssistantPlugin::_send_ai_request(const String &p_message) {
	_add_chat_message("You", p_message);
	
	if (api_key.is_empty()) {
		_add_chat_message("Hyperbeam", "⚠️ Please set your AI API key in the editor settings to use this feature.\n\nGo to Editor > Editor Settings > AI Assistant and add your API key.");
		return;
	}
	
	// Add thinking message
	_add_chat_message("Hyperbeam", "🤔 Thinking...");
	
	// Get current context
	String context = _get_current_editor_context();
	String system_prompt = _build_system_prompt();
	
	// Make API request (simplified for now)
	_make_api_request(p_message, context);
}

void AIAssistantPlugin::_make_api_request(const String &p_prompt, const String &p_context) {
	// This is a simplified implementation
	// In a real implementation, you'd use HTTPClient or HTTPRequest
	// For now, we'll simulate a response
	
	String simulated_response = "I'm a simulated AI response! In a full implementation, I would:\n\n";
	simulated_response += "• Analyze your question: \"" + p_prompt + "\"\n";
	simulated_response += "• Consider the current project context\n";
	simulated_response += "• Provide specific GDScript/C# code examples\n";
	simulated_response += "• Offer game development best practices\n\n";
	simulated_response += "To enable real AI responses, implement the HTTP client integration with your preferred AI service (OpenAI, Anthropic, etc.)";
	
	// Remove the "thinking" message and add the real response
	chat_display->clear();
	for (int i = 0; i < chat_history.size() - 1; i++) {
		chat_display->append_text(chat_history[i]);
	}
	
	_handle_ai_response(simulated_response);
}

void AIAssistantPlugin::_handle_ai_response(const String &p_response) {
	_add_chat_message("Hyperbeam", p_response);
}

void AIAssistantPlugin::_add_chat_message(const String &p_sender, const String &p_message) {
	String formatted_message;
	
	if (p_sender == "You") {
		formatted_message = "[color=lightblue][b]You:[/b][/color] " + p_message + "\n\n";
	} else {
		formatted_message = "[color=lightgreen][b]Hyperbeam:[/b][/color] " + p_message + "\n\n";
	}
	
	chat_history.append(formatted_message);
	chat_display->append_text(formatted_message);
	
	// Auto-scroll to bottom
	call_deferred("_scroll_to_bottom");
}

String AIAssistantPlugin::_build_system_prompt() {
	String prompt = "You are Hyperbeam AI, an intelligent assistant built into the Hyperbeam game engine (based on Godot). ";
	prompt += "You specialize in game development, GDScript, C#, and Godot/Hyperbeam engine features. ";
	prompt += "Provide helpful, accurate, and practical advice for game developers. ";
	prompt += "Always consider performance, best practices, and maintainable code. ";
	prompt += "When providing code examples, prefer GDScript but also support C# when requested.";
	return prompt;
}

String AIAssistantPlugin::_get_current_editor_context() {
	String context = "Current Context:\n";
	
	// Get current scene
	EditorInterface *editor = get_editor_interface();
	if (editor) {
		Node *edited_scene = editor->get_edited_scene_root();
		if (edited_scene) {
			context += "Current Scene: " + edited_scene->get_name() + "\n";
		}
	}
	
	// Add project info
	context += "Project: " + ProjectSettings::get_singleton()->get_setting("application/config/name", "Unknown") + "\n";
	
	return context;
}

void AIAssistantPlugin::ask_ai_question(const String &p_question) {
	_send_ai_request(p_question);
}

void AIAssistantPlugin::explain_code_selection(const String &p_code) {
	if (p_code.is_empty()) {
		_send_ai_request("Please explain the code patterns commonly used in Godot game development.");
	} else {
		_send_ai_request("Please explain this code:\n\n```\n" + p_code + "\n```");
	}
}

void AIAssistantPlugin::suggest_code_improvements(const String &p_code) {
	if (p_code.is_empty()) {
		_send_ai_request("What are some general code improvement tips for Godot/GDScript development?");
	} else {
		_send_ai_request("Please suggest improvements for this code:\n\n```\n" + p_code + "\n```");
	}
}

void AIAssistantPlugin::get_help_with_error(const String &p_error_message, const String &p_context) {
	String message = "I'm getting this error:\n\n" + p_error_message;
	if (!p_context.is_empty()) {
		message += "\n\nContext:\n" + p_context;
	}
	message += "\n\nCan you help me understand and fix it?";
	_send_ai_request(message);
}

void AIAssistantPlugin::set_api_key(const String &p_key) {
	api_key = p_key;
	
	// Save to editor settings
	EditorSettings *settings = EditorSettings::get_singleton();
	if (settings) {
		settings->set_setting("ai_assistant/api_key", p_key);
		settings->save();
	}
}

void AIAssistantPlugin::set_model_name(const String &p_model) {
	model_name = p_model;
	
	// Save to editor settings
	EditorSettings *settings = EditorSettings::get_singleton();
	if (settings) {
		settings->set_setting("ai_assistant/model_name", p_model);
		settings->save();
	}
}

// AI Code Completion Provider Implementation
void AICodeCompletionProvider::request_completion(const String &p_code, int p_cursor_pos) {
	if (!plugin) {
		return;
	}
	
	// Extract context around cursor
	String before_cursor = p_code.substr(0, p_cursor_pos);
	String after_cursor = p_code.substr(p_cursor_pos);
	
	String completion_request = "Provide code completion suggestions for this GDScript code:\n\n";
	completion_request += before_cursor + "[CURSOR]" + after_cursor;
	
	plugin->ask_ai_question(completion_request);
}

void AICodeCompletionProvider::analyze_code_context(const String &p_code) {
	if (!plugin) {
		return;
	}
	
	plugin->explain_code_selection(p_code);
}

// AI Chat Interface Implementation
void AIChatInterface::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_message", "sender", "content"), &AIChatInterface::add_message);
	ClassDB::bind_method(D_METHOD("clear_chat"), &AIChatInterface::clear_chat);
}

AIChatInterface::AIChatInterface() {
	set_name("AIChatInterface");
}

void AIChatInterface::add_message(const String &p_sender, const String &p_content) {
	// Implementation would go here
}

void AIChatInterface::clear_chat() {
	// Implementation would go here
}

// AI Context Analyzer Implementation
void AIContextAnalyzer::analyze_current_project() {
	current_context.project_name = ProjectSettings::get_singleton()->get_setting("application/config/name", "Unknown");
	
	// Analyze scene files
	// This would scan the project for .tscn files
	
	// Analyze script files
	// This would scan the project for .gd and .cs files
	
	// Get current scene and script
	// This would get the currently opened files in the editor
}

String AIContextAnalyzer::get_context_summary() {
	String summary = "Project: " + current_context.project_name + "\n";
	summary += "Scene files: " + String::num_int64(current_context.scene_files.size()) + "\n";
	summary += "Script files: " + String::num_int64(current_context.script_files.size()) + "\n";
	return summary;
}

String AIContextAnalyzer::get_relevant_documentation(const String &p_topic) {
	// This would search through Godot documentation for relevant information
	return "Documentation for: " + p_topic;
}

PackedStringArray AIContextAnalyzer::get_available_nodes() {
	PackedStringArray nodes;
	// This would return all available node types in Godot
	nodes.append("Node");
	nodes.append("Node2D");
	nodes.append("Node3D");
	nodes.append("Control");
	nodes.append("RigidBody2D");
	nodes.append("CharacterBody2D");
	// ... etc
	return nodes;
}

PackedStringArray AIContextAnalyzer::get_available_methods(const String &p_class_name) {
	PackedStringArray methods;
	// This would return available methods for the given class
	if (p_class_name == "Node") {
		methods.append("_ready()");
		methods.append("_process(delta)");
		methods.append("get_node(path)");
		methods.append("queue_free()");
	}
	return methods;
}
