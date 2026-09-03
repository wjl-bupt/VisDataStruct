#pragma once
#include <vector>
#include <stack>
#include <algorithm>
#include <functional>


namespace sortedlist{
    class SortAlgorithms{

        public:
            SortAlgorithms() {}

            // 冒泡排序:相邻比较,逆序即交换,每轮把当前最大值"冒泡"到尾部。
            // 非递归;时间 O(n²),空间 O(1)。
            // 回调:onCompare(i, j) 每次相邻比较;onSwap(i, j) 交换之后;onSorted(idx) 每轮尾部就位
            std::vector<int> BubbleSortAlgo(std::vector<int> &nums,
                    const std::function<void(int,int)>& onCompare = nullptr,
                    const std::function<void(int,int)>& onSwap = nullptr,
                    const std::function<void(int)>& onSorted = nullptr){
                int n = nums.size();
                for(int i = 0; i < n; i++){
                    for(int j = 0; j < n - i - 1; j++){
                        if(onCompare) onCompare(j, j + 1);      // 没交换的比较也要上报
                        if(nums[j] > nums[j + 1]){
                            std::swap(nums[j], nums[j + 1]);
                            if(onSwap) onSwap(j, j + 1);        // 交换之后回调(场景同步元素位置)
                        }
                    }
                    if(onSorted) onSorted(n - i - 1);
                }
                return nums;
            }

            // bubble improve:记录每轮"最后一次交换的位置",其后元素必然已就位,下一轮只需扫到这里。
            // 对基本有序的数组可提前结束:最坏 O(n²),最好(已有序)一轮扫完 O(n)。空间 O(1),非递归。
            // 回调:onCompare(i, j) 每次比较(含未交换的);onSwap(i, j) 交换之后;onSorted(idx) 下标 idx 就位。
            // 就位上报规则(不重复):每轮新就位的只有 [lastswap, sorted_hi) 这一段,只上报这段;
            // 循环结束后补报前缀 [0, n)。
            std::vector<int> BubbleSortAlgoImprovement(std::vector<int> &nums,
                const std::function<void(int, int)> &onCompare = nullptr,
                const std::function<void(int, int)> &onSwap = nullptr,
                const std::function<void(int)> &onSorted = nullptr){

                int len = nums.size();
                int n = len;                      // [0, n) 是尚未确认有序的前缀
                int sorted_hi = len;              // [sorted_hi, len) 已上报就位
                while(n > 1){
                    int lastswap = 0;             // 本轮最后一次交换的位置(0 = 本轮无交换)
                    for(int j = 0; j < n - 1; j++){
                        if(onCompare) onCompare(j, j + 1);
                        if(nums[j] > nums[j + 1]){
                            std::swap(nums[j], nums[j + 1]);
                            if(onSwap) onSwap(j, j + 1);
                            lastswap = j + 1;     // ★ 关键:交换发生在 j+1,其后本轮已换有序
                        }
                    }
                    n = lastswap;                 // 下一轮只需排 [0, lastswap)
                    for(int s = n; s < sorted_hi; s++){   // 只上报新就位的那一段
                        if(onSorted) onSorted(s);
                    }
                    sorted_hi = n;
                }
                for(int s = 0; s < n; s++){       // 收尾:前缀 [0, n) 也全部就位
                    if(onSorted) onSorted(s);
                }
                return nums;

            }
            
            // 选择排序:每轮在无序区 [0, n-i-1] 里选最大,放到尾部。
            // 非递归;时间 O(n²)(比较次数固定),交换最多 n-1 次。空间 O(1)。
            // 回调:onCompare(j, pivot) 与当前最优者比较;onSwap(i, j) 交换之后;onSorted(idx) 尾部就位
            std::vector<int> SelectionSortAlgo(std::vector<int> &nums,
                    const std::function<void(int,int)>& onCompare = nullptr,
                    const std::function<void(int,int)>& onSwap = nullptr,
                    const std::function<void(int)>& onSorted = nullptr){
                int n = nums.size();
                for(int i=0;i<n;i++){
                    int pivot = 0;
                    for(int j=1;j<n-i;j++){
                        if(onCompare) onCompare(j, pivot);
                        pivot = (nums[j] > nums[pivot])?j:pivot;
                    }
                    std::swap(nums[n-i-1], nums[pivot]);
                    if(onSwap) onSwap(n-i-1, pivot);         // 交换之后回调
                    if(onSorted) onSorted(n-i-1);
                }
                return nums;
            }

            // 插入排序:把 key 拿出来,比它大的逐个右移,最后插入空位。
            // 非递归;时间最坏 O(n²)、最好(已序)O(n),空间 O(1)。
            // 回调约定(与交换类排序不同,场景按此语义处理):
            //   onCompare(keyIdx, j) —— key(原下标 keyIdx)与 nums[j] 比较;keyIdx 用来标识悬浮的 key
            //   onSwap(j, j + 1)     —— nums[j] 右移一格到 j+1(是"移动"不是交换)
            //   onSorted(i)          —— 第 i 轮结束,前缀 [0, i] 已有序
            std::vector<int> InsertionSortAlgo(std::vector<int> &nums,
                    const std::function<void(int,int)>& onCompare = nullptr,
                    const std::function<void(int,int)>& onSwap = nullptr,
                    const std::function<void(int)>& onSorted = nullptr){

                int n = nums.size();
                for (int i = 1; i < n; i++) {
                    int key = nums[i];
                    int j = i - 1;
                    while (j >= 0) {
                        if(onCompare) onCompare(i, j);       // key 与 nums[j] 比较
                        if (nums[j] <= key) break;
                        nums[j + 1] = nums[j];               // 右移一格
                        if(onSwap) onSwap(j, j + 1);
                        j--;
                    }
                    nums[j + 1] = key;                       // key 落位
                    if(onSorted) onSorted(i);                // 前缀 [0, i] 有序
                }

                return nums;
            }

            std::vector<int> ShellSortAlgo(std::vector<int> &nums){
                return nums;
            }

            // 归并排序(自底向上,非递归):width = 1,2,4,... 相邻段两两归并。
            // 时间 O(n log n),空间 O(n)(temp 辅助数组)。
            // 回调:
            //   onSplit(l, m, r) —— 段 [l,m) 与 [m,r) 即将归并(可视化画区间带)
            //   onCompare(i, j)  —— 归并时取两段头部比较
            //   onWrite(k, val)  —— 写回:值 val 落到下标 k(可视化让对应柱子滑入)
            std::vector<int> MergeSortAlgo(std::vector<int> &nums,
                    const std::function<void(int,int,int)>& onSplit = nullptr,
                    const std::function<void(int,int)>& onCompare = nullptr,
                    const std::function<void(int,int)>& onWrite = nullptr){

                int n = nums.size();
                std::vector<int> temp(n, 0);
                for(int width=1; width<n; width *= 2){
                    for(int left=0; left <n; left += 2*width){
                        int mid = std::min(left + width, n), right = std::min(left + 2 * width, n);
                        if(onSplit) onSplit(left, mid, right);
                        int i=left, j=mid, k=left;
                        while(i< mid && j < right){
                            if(onCompare) onCompare(i, j);
                            if(nums[i] < nums[j]){
                                temp[k++] = nums[i++];
                            }
                            else{
                                temp[k++] = nums[j++];
                            }
                        }

                        while(i<mid) temp[k++] = nums[i++];
                        while(j<right) temp[k++] = nums[j++];

                        // rewirte temp to nums;
                        k=left;
                        while(k < right) {
                            nums[k] = temp[k];
                            if(onWrite) onWrite(k, nums[k]);
                            k++;
                        }
                    }
                }
                return nums;
            }

            // 快速排序(非递归):显式栈保存待排序区间;每轮先处理小区间、大区间入栈,
            // 栈深最坏也只有 O(log n)。Lomuto 分区:取区间末元素为基准(可视化语义清晰:
            // 基准固定在末尾被扫描,落位即全局有序)。
            // 平均时间 O(n log n),最坏 O(n²)(已序数组 + 末尾基准会退化——适用边界);
            // 空间 O(log n)。不稳定(相等元素相对顺序会改变)。
            // 回调:onRange(lo, hi) 当前处理的子区间确定/收缩时(可视化框出子区间);
            //       onCompare(j, hi) 与基准比较;onSwap(i, j) 交换之后;onSorted(idx) 基准落位(全局有序)
            std::vector<int> QuickSortAlgo(std::vector<int> &nums,
                    const std::function<void(int,int)>& onRange = nullptr,
                    const std::function<void(int,int)>& onCompare = nullptr,
                    const std::function<void(int,int)>& onSwap = nullptr,
                    const std::function<void(int)>& onSorted = nullptr){

                const int n = nums.size();
                std::vector<std::pair<int,int>> st;
                if (n > 0) st.push_back({0, n - 1});
                while (!st.empty()) {
                    int lo = st.back().first, hi = st.back().second;
                    st.pop_back();
                    if(onRange) onRange(lo, hi);
                    while (lo < hi) {
                        int i = lo;                       // 小于基准的边界
                        for (int j = lo; j < hi; ++j) {
                            if(onCompare) onCompare(j, hi);
                            if (nums[j] < nums[hi]) {
                                if (i != j) {
                                    std::swap(nums[i], nums[j]);
                                    if(onSwap) onSwap(i, j);
                                }
                                ++i;
                            }
                        }
                        if (i != hi) {
                            std::swap(nums[i], nums[hi]);
                            if(onSwap) onSwap(i, hi);
                        }
                        if(onSorted) onSorted(i);         // 基准落位,全局有序
                        if (i - lo < hi - i) {            // 先处理小的一半:栈深 O(log n)
                            if (i + 1 <= hi) st.push_back({i + 1, hi});   // 注意 <=:单元素区间也要上报
                            hi = i - 1;
                            if (lo <= hi && onRange) onRange(lo, hi);     // 基准左侧的剩余子区间
                        } else {
                            if (lo <= i - 1) st.push_back({lo, i - 1});
                            lo = i + 1;
                            if (lo <= hi && onRange) onRange(lo, hi);     // 基准右侧的剩余子区间
                        }
                    }
                    if (lo == hi && onSorted) onSorted(lo);   // 单元素区间天然有序
                }
                return nums;
            }

            std::vector<int> HeapSortAlgo(std::vector<int> &nums){
                return nums;
            }

            // 计数排序(非比较排序):统计每个值出现次数,按值从小到大写回。
            // 时间 O(n + k),空间 O(n + k),k = 值域宽度(值域大时空间不可行——适用边界)。
            // 非递归。负数支持:按下标偏移(原实现把负数改写成 0 会静默破坏数据,已修正)。
            // 回调:onCount(i) 扫描下标 i 计数;onWrite(pos, val) 值 val 写回下标 pos
            std::vector<int> CountingSortAlgo(std::vector<int> &nums,
                    const std::function<void(int)>& onCount = nullptr,
                    const std::function<void(int,int)>& onWrite = nullptr){

                int n = nums.size();
                if(n <= 1) return nums;
                int mn = nums[0], mx = nums[0];       // 偏移到 0 起:负数无需改写数据
                for(int i=0;i<n;i++){
                    if(nums[i] < mn) mn = nums[i];
                    if(nums[i] > mx) mx = nums[i];
                }
                std::vector<int> counting(mx - mn + 1, 0);
                for(int i=0;i<n;i++){
                    if(onCount) onCount(i);
                    counting[nums[i] - mn]++;
                }
                int k = 0;
                for(int i=0;i<(mx-mn+1);i++){
                    while(counting[i] > 0){
                        nums[k] = i + mn;             // 写回时偏移回真实值
                        if(onWrite) onWrite(k, nums[k]);
                        k++;
                        counting[i]--;
                    }
                }
                return nums;
            }

            std::vector<int> RadixSortAlgo(std::vector<int> &nums){
                return nums;
            }

            std::vector<int> BucketSortAlgo(std::vector<int> &nums){
                return nums;
            }

    };
}