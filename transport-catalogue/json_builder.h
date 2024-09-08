#pragma once
#include "json.h"

#include <string>
#include <vector>

namespace json {
	class Builder {
		class BaseContext;
		class DictItemContext;
		class DictValueContext;
		class ArrayItemContext;
	public:
		Builder() = default;
		DictValueContext Key(std::string key);
		BaseContext Value(Node::Value value);
		DictItemContext StartDict();
		ArrayItemContext StartArray();
		BaseContext EndDict();
		BaseContext EndArray();
		Node Build();
	private:
		Node root_;
		std::vector<Node*> nodes_stack_;
		bool is_key_added_ = false;

		void StartNode(Node node);

		class BaseContext {
		public:
			BaseContext(Builder& builder)
				:builder_(builder) {}

			DictValueContext Key(std::string key) {
				return builder_.Key(std::move(key));
			}

			BaseContext Value(Node::Value value) {
				return builder_.Value(std::move(value));
			}

			DictItemContext StartDict() {
				return builder_.StartDict();
			}
			ArrayItemContext StartArray() {
				return builder_.StartArray();
			}
			BaseContext EndDict() {
				return builder_.EndDict();
			}
			BaseContext EndArray() {
				return builder_.EndArray();
			}
			Node Build() {
				return builder_.Build();
			}
		private:
			Builder& builder_;
		};

		class DictItemContext : public BaseContext {
		public:
			DictItemContext(BaseContext base)
				:BaseContext(base) {}
			BaseContext Value(Node::Value value) = delete;
			DictItemContext StartDict() = delete;
			ArrayItemContext StartArray() = delete;
			BaseContext EndArray() = delete;
			Node Build() = delete;
		};

		class DictValueContext : public BaseContext {
		public:
			DictValueContext(BaseContext base)
				:BaseContext(base) {}
			DictValueContext Key(std::string key) = delete;
			DictItemContext Value(Node::Value value) {
				return BaseContext::Value(std::move(value));
			}
			BaseContext EndDict() = delete;
			BaseContext EndArray() = delete;
			Node Build() = delete;

		};

		class ArrayItemContext : public BaseContext {
		public:
			ArrayItemContext(BaseContext base)
				:BaseContext(base) {}
			DictValueContext Key(std::string key) = delete;
			ArrayItemContext Value(Node::Value value) {
				return BaseContext::Value(std::move(value));
			}
			BaseContext EndDict() = delete;
			Node Build() = delete;
		};

	};
}

