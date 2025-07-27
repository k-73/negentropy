#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../Diagram/Component.hpp"

namespace UI {
    struct TreeNode {
        std::string name;
        std::string type;
        Diagram::Component* component = nullptr;
        std::vector<std::unique_ptr<TreeNode>> children;
        bool expanded = true;

        TreeNode(std::string n, std::string t, Diagram::Component* c = nullptr) 
            : name(std::move(n)), type(std::move(t)), component(c) {}

        void AddChild(std::unique_ptr<TreeNode> child) {
            children.push_back(std::move(child));
        }
    };

    class ComponentHierarchy {
    public:
        void BuildFromBlocks(const std::vector<Diagram::Block>& blocks) {
            m_root = std::make_unique<TreeNode>("Scene", "Root");
            
            auto blocksNode = std::make_unique<TreeNode>("Blocks", "Group");
            for (size_t i = 0; i < blocks.size(); ++i) {
                auto blockNode = std::make_unique<TreeNode>(
                    blocks[i].data.label.empty() ? "Block " + std::to_string(i + 1) : blocks[i].data.label,
                    "Block", 
                    const_cast<Diagram::Block*>(&blocks[i])
                );
                blocksNode->AddChild(std::move(blockNode));
            }
            m_root->AddChild(std::move(blocksNode));
        }

        TreeNode* GetRoot() const noexcept { return m_root.get(); }

    private:
        std::unique_ptr<TreeNode> m_root;
    };
}