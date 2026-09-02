#pragma once
#include <iostream>
#include <stack>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>
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
                if(!root) return;          // 空树直接返回(否则下面解引用空指针)
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
                root = nullptr; depth = 0; width = 0;   // 真正重置(原来的 BinaryTree(); 只是构造临时对象)

                return;
            }

            // 通过满二叉树列表建立二叉树(层序数组,空位传 nullptr)
            // onAttach: 每挂接一个节点回调一次 (parent, child);根节点时 parent 为 nullptr。可视化用,可不传
            TreeNode* createbinarytree(vector<TreeNode*> nodes,
                                       const function<void(TreeNode*, TreeNode*)>& onAttach = nullptr){
                int n = nodes.size();
                clear();
                if(n <= 0) return root;
                queue<TreeNode*> st;            // 队列:出队顺序 = 下标顺序,cur_index 才能对上
                root = nodes[0];
                TreeNode *node = root;
                st.push(root);
                if(onAttach) onAttach(nullptr, root);
                int cur_index = 0;
                while(!st.empty()){
                    node = st.front();
                    st.pop();
                    int l = 2*cur_index + 1, r = 2*cur_index + 2;   // 0 起始数组:孩子是 2i+1 / 2i+2
                   if(l < n && nodes[l] != nullptr){
                        node->lchild = nodes[l];
                        st.push(node->lchild);
                        if(onAttach) onAttach(node, node->lchild);
                   }
                   if(r < n && nodes[r] != nullptr){
                        node->rchild = nodes[r];
                        st.push(node->rchild);
                        if(onAttach) onAttach(node, node->rchild);
                   }
                    cur_index++;
                }
                return root;
            }

            TreeNode* getRoot() const { return root; }   // 场景需要读根指针

            // 二叉树前序遍历: root->lchild->rchild
            // onVisit: 每访问一个节点回调一次(把节点指针交出去)。可视化用,可不传
            vector<int> PreOrderTraversal(const function<void(TreeNode*)>& onVisit = nullptr){
                vector<int> ans;
                if(!root) return ans;
                stack<TreeNode*> st;
                st.push(root);
                while(!st.empty()){
                    TreeNode *node = st.top();
                    st.pop(); ans.push_back(node->val);
                    if(onVisit) onVisit(node);
                    if(node->rchild) st.push(node->rchild);
                    if(node->lchild) st.push(node->lchild);
                }

                return ans;
            }

            // 二叉树中序遍历: lchild->root -> rchild
            vector<int> InOrderTraversal(const function<void(TreeNode*)>& onVisit = nullptr){
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
                        if(onVisit) onVisit(node);
                        node = node->rchild;
                    }

                }

                return ans;
            }

            // postorder Traversal()
            // 技巧:按 根->右->左 访问,结果再 reverse 即为后序。
            // 注意 onVisit 回调发生在 reverse 之前(即 根右左 顺序),可视化侧需自行逆序回放
            vector<int> PostorderTraversal(const function<void(TreeNode*)>& onVisit = nullptr){
                vector<int> ans;
                if(!root) return ans;
                stack<TreeNode*> st;
                st.push(root);
                while(!st.empty()){
                    TreeNode *node = st.top();
                    st.pop(); ans.push_back(node->val);
                    if(onVisit) onVisit(node);
                    if(node->lchild) st.push(node->lchild);
                    if(node->rchild) st.push(node->rchild);
                }
                reverse(ans.begin(), ans.end());
                return ans;
            }

            // level order
            vector<int> LevelOrderTraversal(const function<void(TreeNode*)>& onVisit = nullptr){
                vector<int> ans;
                if(!root) return ans;
                queue<TreeNode*> que;
                que.push(root);
                while(!que.empty()){
                    TreeNode *node = que.front(); que.pop();
                    if(node->lchild) que.push(node->lchild);
                    if(node->rchild) que.push(node->rchild);
                    ans.push_back(node->val);
                    if(onVisit) onVisit(node);
                }

                return ans;
            }

            // 镜像翻转:交换每个节点的左右子树。
            // 非递归:显式栈 DFS。时间 O(n)(每节点交换一次),空间 O(h)(h = 树高,栈里最多一条路径的待访问节点)
            // onSwap(node):每交换完一个节点的左右孩子回调一次,可视化用
            void invert(const function<void(TreeNode*)>& onSwap = nullptr){
                if(!root) return;
                vector<TreeNode*> st;
                st.push_back(root);
                while(!st.empty()){
                    TreeNode *node = st.back(); st.pop_back();
                    swap(node->lchild, node->rchild);      // 出栈即交换
                    if(onSwap) onSwap(node);
                    if(node->lchild) st.push_back(node->lchild);   // 交换后原右子树现在在左边
                    if(node->rchild) st.push_back(node->rchild);
                }
            }

            // height of tree:逐层 BFS 层数。onVisit(node, level) 每访问一个节点回调一次,可视化用
            int GetDepth(const function<void(TreeNode*, int)>& onVisit = nullptr){
                if(!root) return 0;          // 空树保护(否则空指针入队后解引用崩溃)
                queue<TreeNode*> que;
                que.push(root);
                depth = 0;
                while(!que.empty()){
                    int level_width = que.size();
                    for(int i =0; i<level_width; i++){
                        TreeNode *node = que.front(); que.pop();
                        if(onVisit) onVisit(node, depth);
                        if(node->lchild) que.push(node->lchild);
                        if(node->rchild) que.push(node->rchild);
                    }
                    depth ++;
                }
                return depth;
            }

            // width of tree:同一层最左/最右非空节点的下标跨度(中间空位 # 算在跨度内),各层取最大。
            // 节点下标:root=0,孩子 2i+1 / 2i+2。例:某层节点在下标 3 和 6 → 该层宽 6-3+1 = 4(非空只有 2 个)。
            // 实现思路:BFS 生成下一层孩子时,第一个生成的下标即最左、最后一个即最右(BFS 生成顺序严格递增)。
            // onVisit(node, index) 每访问一个节点回调一次,可视化用
            int GetWidth(const function<void(TreeNode*, long long)>& onVisit = nullptr){
                if(!root) return 0;
                width = 1;
                queue<pair<TreeNode*, long long>> que;   // long long:斜树下标按 2^深度 指数增长,int 会溢出
                que.push(make_pair(root, 0LL));
                while(!que.empty()){
                    int n = que.size();
                    int ldx=-1, rdx=-1;
                    for(int i=0;i<n;i++){
                        TreeNode *node = que.front().first;
                        long long cidx = que.front().second;   // long long:与队列一致,防下标溢出
                        que.pop();
                        if(onVisit) onVisit(node, cidx);
                        if(node->lchild) {
                            que.push({node->lchild, 2*cidx+1});
                            if(ldx == -1) ldx = 2*cidx+1;
                            if(rdx == -1 || rdx < 2*cidx+1) rdx = 2*cidx + 1;
                        }
                        if(node->rchild){
                            que.push({node->rchild, 2*cidx+2});
                            if(ldx == -1) ldx = 2*cidx+2;
                            if(rdx == -1 || rdx < 2*cidx+2) rdx = 2*cidx + 2;
                        }
                    }

                    width = width > (rdx-ldx+1)? width:(rdx-ldx+1);

                }
                return width;
            }

            // 最近公共祖先 LCA(一般二叉树,非递归)。
            // 思路:迭代 DFS 求 root→p、root→q 两条路径,再比对共同前缀,最后一个相同节点即 LCA。
            // 时间 O(n)(两次 DFS 各访问每节点至多一次),空间 O(h)(栈 + 两条路径)。
            // 注:若树恰好是 BST,可从根往下比较大小走,O(h) 时间 O(1) 空间;这里按一般二叉树实现。
            // pathP/pathQ 非空时回填两条路径(含两端),供可视化使用。
            // onStep(node, phase):phase 0=正在找 p 的路径 1=正在找 q 的路径 2=正在比对路径,每步回调一次
            TreeNode* LCA(TreeNode* p, TreeNode* q,
                          vector<TreeNode*>* pathP = nullptr, vector<TreeNode*>* pathQ = nullptr,
                          const function<void(TreeNode*, int)>& onStep = nullptr){
                if(!p || !q) return nullptr;
                vector<TreeNode*> pp, qq;
                bool okp = findPath(p, &pp, [&](TreeNode* n){ if(onStep) onStep(n, 0); });
                bool okq = findPath(q, &qq, [&](TreeNode* n){ if(onStep) onStep(n, 1); });
                if(pathP) *pathP = pp;
                if(pathQ) *pathQ = qq;
                if(!okp || !okq) return nullptr;

                TreeNode *lca = nullptr;
                size_t m = min(pp.size(), qq.size());
                for(size_t i = 0; i < m; ++i){
                    if(pp[i] != qq[i]) break;       // 第一个分叉处,上一个相同节点即 LCA
                    lca = pp[i];
                    if(onStep) onStep(pp[i], 2);
                }
                return lca;
            }

        private:
            // 迭代 DFS 求 root→target 的路径(含两端),写入 path,找到返回 true。
            // 显式栈 <节点, 状态:0 试左 1 试右 2 出栈>,空间 O(h)
            bool findPath(TreeNode* target, vector<TreeNode*>* path,
                          const function<void(TreeNode*)>& onStep){
                if(!root || !target) return false;
                vector<pair<TreeNode*, int>> st;
                st.push_back({root, 0});
                if(onStep) onStep(root);
                while(!st.empty()){
                    TreeNode *node = st.back().first;
                    int state = st.back().second;
                    if(node == target){
                        if(path){ path->clear(); for(auto &pr : st) path->push_back(pr.first); }
                        return true;
                    }
                    TreeNode *next = nullptr;
                    if(state == 0){ st.back().second = 1; next = node->lchild; }
                    else if(state == 1){ st.back().second = 2; next = node->rchild; }
                    else { st.pop_back(); continue; }
                    if(next){
                        st.push_back({next, 0});    // push 后 st.back() 已变,下一轮重新取
                        if(onStep) onStep(next);
                    }
                }
                return false;
            }

            

    };

}