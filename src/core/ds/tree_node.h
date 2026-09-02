#pragma once
#include <iostream>
#include <stack>
#include <vector>
#include <queue>
#include <algorithm>
using namespace std;

namespace ds{
    struct TreeNode {
        int val;
        TreeNode *lchild, *rchild;

        TreeNode() : val(-1), lchild(nullptr), rchild(nullptr) {}

        TreeNode(int v) : val(v), lchild(nullptr), rchild(nullptr) {}
    };


    class BinaryTree{
        private:
            TreeNode *root;
            int depth = 0, width = 0;
        public:
            BinaryTree(){root = nullptr; depth = 0; width = 0;}

            void clear(){
                stack<TreeNode*> st;
                TreeNode *node = root;
                st.push(node);
                while(!st.empty()){
                    int n = st.size();
                    for(int i=0;i <n;i++){
                        node = st.top(); st.pop();
                        if(node->lchild) st.push(node->lchild);
                        if(node->rchild) st.push(node->rchild);
                        delete node;
                    }
                }
                BinaryTree();

                return;
            }

            // 通过满二叉树列表建立二叉树列表
            TreeNode* createbinarytree(vector<TreeNode*> nodes){
                // we assume all number in tree is larger than zero.
                int n = nodes.size();
                clear();
                if(n <= 0) return root;
                stack<TreeNode*> st;
                root = nodes[0];
                TreeNode *node = root;
                st.push(root);
                int cur_index = 0;
                while(!st.empty()){
                    node = st.top();
                    st.pop();
                   if(2*cur_index < n && nodes[2*cur_index] != nullptr){
                        node->lchild = nodes[2*cur_index];
                        st.push(node->lchild);
                   } 
                   if((2*cur_index + 1 < n) && nodes[2*cur_index+1] != nullptr){
                        node->rchild = nodes[2*cur_index+1];
                        st.push(node->rchild);
                   }
                    cur_index++;
                }
                return root;
            }

            // 二叉树前序遍历: root->lchild->rchild
            vector<int> PreOrderTraversal(){
                vector<int> ans;
                if(!root) return ans;
                stack<TreeNode*> st;
                st.push(root);
                while(!st.empty()){
                    TreeNode *node = st.top();
                    st.pop(); ans.push_back(node->val);
                    if(node->rchild) st.push(node->rchild);
                    if(node->lchild) st.push(node->lchild);
                }

                return ans;
            }

            // 二叉树中序遍历: lchild->root -> rchild
            vector<int> InOrderTraversal(){
                vector<int> ans;
                if(!root) return ans;
                stack<TreeNode*> st;
                TreeNode *node = root;
                while(node || !st.empty()){
                    while(node){
                        st.push(node);
                        node = node->lchild;
                    }
                    
                    if(!st.empty()){
                        node = st.top(); st.pop();
                        ans.push_back(node->val); 
                        node = node->rchild;
                    }

                }

                return ans;
            }

            // postorder Traversal()
            vector<int> PostorderTraversal(){
                vector<int> ans;
                if(!root) return ans;
                stack<TreeNode*> st;
                st.push(root);
                while(!st.empty()){
                    TreeNode *node = st.top();
                    st.pop(); ans.push_back(node->val);
                    if(node->lchild) st.push(node->lchild);
                    if(node->rchild) st.push(node->rchild);
                }
                reverse(ans.begin(), ans.end());
                return ans;
            }

            // level order
            vector<int> LevelOrderTraversal(){
                vector<int> ans;
                if(!root) return ans;
                queue<TreeNode*> que;
                que.push(root);
                while(!que.empty()){
                    TreeNode *node = que.front(); que.pop();
                    if(node->lchild) que.push(node->lchild);
                    if(node->rchild) que.push(node->rchild);
                    ans.push_back(node->val);
                }

                return ans;
            }

            // height of tree
            int GetDepth(){
                if(root){
                    if(depth > 0){
                        return depth;
                    }
                    else{
                        vector<int> ans;
                        queue<TreeNode*> que;
                        que.push(root);
                        depth = 0;
                        while(!que.empty()){
                            int level_width = que.size();
                            for(int i =0; i<level_width; i++){
                                TreeNode *node = que.front(); que.pop();
                                if(node->lchild) que.push(node->lchild);
                                if(node->rchild) que.push(node->rchild);
                                ans.push_back(node->val);
                            }
                            depth ++;
                        }

                        return depth;
                    }
                }
                return 0;
            }

            // width of tree
    };

}