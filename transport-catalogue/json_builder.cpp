#include "json_builder.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

using namespace json;

Builder::DictValueContext Builder::Key(std::string key) {
	if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap()) {
		throw std::logic_error("invalid context");
	}
	Node* new_dict_val = &std::get<Dict>(nodes_stack_.back()->GetValue())[key];
	nodes_stack_.emplace_back(new_dict_val);
	is_key_added_ = true;
	return DictValueContext{ *this };
}

Builder::BaseContext Builder::Value(Node::Value value) {
	if (nodes_stack_.empty() && root_.IsNull()) {
		root_ = value;
	}
	else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
		std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back(std::move(value));

	}
	else if (is_key_added_) {
		*nodes_stack_.back() = value;
		nodes_stack_.pop_back();
		is_key_added_ = false;

	}
	return BaseContext{ *this };
}

void Builder::StartNode(Node node) {
	if (root_.IsNull()) {
		root_ = node;
		nodes_stack_.emplace_back(&root_);
	}
	else if (is_key_added_) {
		*nodes_stack_.back() = node;
		is_key_added_ = false;
	}
	else if (!nodes_stack_.empty() && nodes_stack_.back()->IsArray()) {
		Node* node_ptr = &std::get<Array>(nodes_stack_.back()->GetValue()).emplace_back(std::move(node));
		nodes_stack_.emplace_back(node_ptr);
	}

}

Builder::DictItemContext Builder::StartDict() {
	StartNode(Node{ Dict{} });
	return DictItemContext{ *this };
}

Builder::ArrayItemContext Builder::StartArray() {
	StartNode(Node{ Array{} });
	return ArrayItemContext{ *this };
}

Builder::BaseContext Builder::EndDict() {

	if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap()) {
		throw std::logic_error("invalid context");
	}
	nodes_stack_.pop_back();
	return BaseContext{ *this };
}
Builder::BaseContext Builder::EndArray() {
	if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
		throw std::logic_error("invalid context");
	}
	nodes_stack_.pop_back();
	return BaseContext {*this };
}

Node Builder::Build() {
	if (root_.IsNull() || !nodes_stack_.empty()) {
		throw std::logic_error("invalid context");
	}
	return root_;

}
