/**
 * MemTracer 使用示例
 * 
 * 编译方法（需要先编译项目并启用MemTracer）:
 *   g++ -std=c++20 -I../deps -o memtracer_example memtracer_example.cpp -L../build/lib -lmemtracer -pthread
 * 
 * 运行方法:
 *   LD_LIBRARY_PATH=../build/lib ./memtracer_example
 * 
 * 或者使用LD_PRELOAD方式:
 *   LD_PRELOAD=../build/lib/libmemtracer.so ./memtracer_example
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "memtracer/mt_info.h"

using namespace std;

// 格式化字节数为可读格式
string format_bytes(size_t bytes) {
    if (bytes < 1024) {
        return to_string(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return to_string(bytes / 1024) + " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        return to_string(bytes / (1024 * 1024)) + " MB";
    } else {
        return to_string(bytes / (1024 * 1024 * 1024)) + " GB";
    }
}

// 打印当前内存使用情况
void print_memory_status(const string& label) {
    size_t allocated = memtracer::allocated_memory();
    size_t meta = memtracer::meta_memory();
    size_t limit = memtracer::memory_limit();
    
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    cout << "📊 " << label << endl;
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    cout << "  已分配内存: " << format_bytes(allocated) 
         << " (" << allocated << " bytes)" << endl;
    cout << "  元数据内存: " << format_bytes(meta) 
         << " (" << meta << " bytes)" << endl;
    
    if (limit > 0) {
        cout << "  内存限制:   " << format_bytes(limit) 
             << " (" << limit << " bytes)" << endl;
        double usage_percent = (double)allocated / limit * 100.0;
        cout << "  使用率:     " << fixed << usage_percent << "%" << endl;
        
        if (usage_percent > 80.0) {
            cout << "  ⚠️  警告: 内存使用率超过80%!" << endl;
        }
    } else {
        cout << "  内存限制:   未设置" << endl;
    }
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    cout << endl;
}

// 示例1: 基本内存分配监控
void example1_basic_allocation() {
    cout << "\n🔹 示例1: 基本内存分配监控\n" << endl;
    
    print_memory_status("初始状态");
    
    // 分配10MB内存
    const size_t size = 10 * 1024 * 1024;
    cout << "分配 " << format_bytes(size) << " 内存..." << endl;
    char* buffer = new char[size];
    
    print_memory_status("分配10MB后");
    
    // 释放内存
    cout << "释放内存..." << endl;
    delete[] buffer;
    
    print_memory_status("释放后");
}

// 示例2: 循环分配和释放
void example2_repeated_allocation() {
    cout << "\n🔹 示例2: 循环分配和释放\n" << endl;
    
    const int iterations = 10;
    const size_t chunk_size = 1024 * 1024;  // 1MB
    
    for (int i = 0; i < iterations; i++) {
        char* buffer = new char[chunk_size];
        
        cout << "迭代 " << (i + 1) << "/" << iterations 
             << " - 已分配: " << format_bytes(memtracer::allocated_memory()) 
             << endl;
        
        // 模拟使用内存
        memset(buffer, i, chunk_size);
        
        // 短暂延迟
        this_thread::sleep_for(chrono::milliseconds(100));
        
        delete[] buffer;
    }
    
    print_memory_status("循环结束后");
}

// 示例3: 模拟内存压力
void example3_memory_pressure() {
    cout << "\n🔹 示例3: 模拟内存压力\n" << endl;
    
    vector<char*> allocations;
    const size_t chunk_size = 5 * 1024 * 1024;  // 5MB per chunk
    const int max_chunks = 10;
    
    try {
        for (int i = 0; i < max_chunks; i++) {
            cout << "分配第 " << (i + 1) << " 块..." << endl;
            
            char* buffer = new char[chunk_size];
            allocations.push_back(buffer);
            
            size_t current = memtracer::allocated_memory();
            size_t limit = memtracer::memory_limit();
            
            cout << "  当前: " << format_bytes(current);
            if (limit > 0) {
                cout << " / " << format_bytes(limit) 
                     << " (" << (current * 100 / limit) << "%)";
            }
            cout << endl;
            
            // 检查是否接近限制
            if (limit > 0 && current > limit * 0.9) {
                cout << "⚠️  警告: 接近内存限制，停止分配" << endl;
                break;
            }
        }
    } catch (const bad_alloc& e) {
        cout << "❌ 内存分配失败: " << e.what() << endl;
    }
    
    print_memory_status("分配完成后");
    
    // 清理
    cout << "清理内存..." << endl;
    for (char* buffer : allocations) {
        delete[] buffer;
    }
    allocations.clear();
    
    print_memory_status("清理后");
}

// 示例4: 模拟外部排序场景
void example4_external_sort_simulation() {
    cout << "\n🔹 示例4: 模拟外部排序场景\n" << endl;
    
    size_t limit = memtracer::memory_limit();
    size_t current = memtracer::allocated_memory();
    
    // 假设我们需要排序的数据总量
    size_t total_data_size = 100 * 1024 * 1024;  // 100MB
    
    // 可用内存（考虑安全系数）
    size_t available_memory = 0;
    if (limit > 0) {
        available_memory = (limit - current) * 0.7;  // 70%安全系数
    } else {
        available_memory = 50 * 1024 * 1024;  // 假设50MB可用
    }
    
    cout << "总数据量: " << format_bytes(total_data_size) << endl;
    cout << "可用内存: " << format_bytes(available_memory) << endl;
    
    if (total_data_size > available_memory) {
        cout << "✓ 需要使用外部排序" << endl;
        
        // 计算需要的run数量
        size_t run_count = (total_data_size + available_memory - 1) / available_memory;
        cout << "  预计生成 " << run_count << " 个run文件" << endl;
        
        // 计算归并路数
        size_t buffer_per_reader = 4096;  // 4KB per reader
        size_t max_merge_ways = available_memory / buffer_per_reader;
        cout << "  最大归并路数: " << max_merge_ways << endl;
        
        // 计算归并轮数
        size_t merge_rounds = 0;
        size_t current_runs = run_count;
        while (current_runs > 1) {
            current_runs = (current_runs + max_merge_ways - 1) / max_merge_ways;
            merge_rounds++;
        }
        cout << "  预计归并轮数: " << merge_rounds << endl;
    } else {
        cout << "✓ 可以使用内存排序" << endl;
    }
}

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════╗
║           MiniOB MemTracer 使用示例                        ║
╚═══════════════════════════════════════════════════════════╝
)" << endl;
    
    // 检查MemTracer是否启用
    size_t initial_mem = memtracer::allocated_memory();
    if (initial_mem == 0 && memtracer::memory_limit() == 0) {
        cout << "⚠️  警告: MemTracer可能未正确加载" << endl;
        cout << "请使用以下方式运行:" << endl;
        cout << "  LD_PRELOAD=../build/lib/libmemtracer.so ./memtracer_example" << endl;
        cout << endl;
    }
    
    print_memory_status("程序启动");
    
    // 运行示例
    example1_basic_allocation();
    example2_repeated_allocation();
    example3_memory_pressure();
    example4_external_sort_simulation();
    
    cout << "\n✅ 所有示例运行完成\n" << endl;
    print_memory_status("程序结束");
    
    return 0;
}

