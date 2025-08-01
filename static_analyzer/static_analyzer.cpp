#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

struct AllocationSite {
    std::string function;
    int line_number;
    std::string variable_name;
    std::string allocation_type; // malloc, calloc, new, etc.
};

struct FreeSite {
    std::string function;
    int line_number;
    std::string variable_name;
    std::string free_type; // free, delete, delete[]
};

struct FunctionInfo {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::string> return_paths;
    bool returns_allocated_memory = false;
};

class MemoryLeakAnalyzer {
private:
    std::vector<AllocationSite> allocations;
    std::vector<FreeSite> frees;
    std::unordered_map<std::string, FunctionInfo> functions;
    std::unordered_set<std::string> leaked_variables;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    // Regex patterns for different allocation/deallocation patterns
    std::regex malloc_pattern{R"(\b(\w+)\s*=\s*(malloc|calloc|realloc)\s*\()"};
    std::regex new_pattern{R"(\b(\w+)\s*=\s*new\s+)"};
    std::regex new_array_pattern{R"(\b(\w+)\s*=\s*new\s+\w+\s*\[)"};
    std::regex free_pattern{R"(\bfree\s*\(\s*(\w+)\s*\))"};
    std::regex delete_pattern{R"(\bdelete\s+(\w+))"};
    std::regex delete_array_pattern{R"(\bdelete\s*\[\s*\]\s*(\w+))"};
    std::regex return_pattern{R"(\breturn\s+(\w+))"};
    std::regex function_pattern{R"((\w+)\s+(\w+)\s*\([^)]*\)\s*\{)"};

public:
    void analyze_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            errors.push_back("Cannot open file: " + filename);
            return;
        }

        std::string line;
        int line_number = 0;
        std::string current_function = "global";

        while (std::getline(file, line)) {
            line_number++;
            
            // Remove comments
            size_t comment_pos = line.find("//");
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }

            // Skip empty lines
            if (line.empty() || std::all_of(line.begin(), line.end(), ::isspace)) {
                continue;
            }

            // Track current function
            std::smatch function_match;
            if (std::regex_search(line, function_match, function_pattern)) {
                current_function = function_match[2].str();
            }

            // Find allocations
            find_allocations(line, line_number, current_function);
            
            // Find deallocations
            find_deallocations(line, line_number, current_function);
            
            // Find return statements
            find_returns(line, line_number, current_function);
        }

        analyze_memory_patterns(filename);
    }

    void analyze_directory(const std::string& directory_path) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(directory_path)) {
                if (entry.is_regular_file()) {
                    const auto& path = entry.path();
                    std::string extension = path.extension().string();
                    
                    // Process C/C++ files
                    if (extension == ".c" || extension == ".cpp" || extension == ".cc" || 
                        extension == ".cxx" || extension == ".h" || extension == ".hpp") {
                        std::cout << "Analyzing: " << path.string() << std::endl;
                        analyze_file(path.string());
                    }
                }
            }
        } catch (const fs::filesystem_error& ex) {
            errors.push_back("Filesystem error: " + std::string(ex.what()));
        }
    }

private:
    void find_allocations(const std::string& line, int line_number, const std::string& function) {
        std::smatch match;
        
        // Check for malloc/calloc/realloc
        if (std::regex_search(line, match, malloc_pattern)) {
            AllocationSite site;
            site.function = function;
            site.line_number = line_number;
            site.variable_name = match[1].str();
            site.allocation_type = match[2].str();
            allocations.push_back(site);
        }
        
        // Check for new
        if (std::regex_search(line, match, new_pattern)) {
            AllocationSite site;
            site.function = function;
            site.line_number = line_number;
            site.variable_name = match[1].str();
            site.allocation_type = "new";
            allocations.push_back(site);
        }
        
        // Check for new[]
        if (std::regex_search(line, match, new_array_pattern)) {
            AllocationSite site;
            site.function = function;
            site.line_number = line_number;
            site.variable_name = match[1].str();
            site.allocation_type = "new[]";
            allocations.push_back(site);
        }
    }

    void find_deallocations(const std::string& line, int line_number, const std::string& function) {
        std::smatch match;
        
        // Check for free
        if (std::regex_search(line, match, free_pattern)) {
            FreeSite site;
            site.function = function;
            site.line_number = line_number;
            site.variable_name = match[1].str();
            site.free_type = "free";
            frees.push_back(site);
        }
        
        // Check for delete
        if (std::regex_search(line, match, delete_pattern)) {
            FreeSite site;
            site.function = function;
            site.line_number = line_number;
            site.variable_name = match[1].str();
            site.free_type = "delete";
            frees.push_back(site);
        }
        
        // Check for delete[]
        if (std::regex_search(line, match, delete_array_pattern)) {
            FreeSite site;
            site.function = function;
            site.line_number = line_number;
            site.variable_name = match[1].str();
            site.free_type = "delete[]";
            frees.push_back(site);
        }
    }

    void find_returns(const std::string& line, int line_number, const std::string& function) {
        std::smatch match;
        if (std::regex_search(line, match, return_pattern)) {
            std::string returned_var = match[1].str();
            
            // Check if this variable was allocated in this function
            for (const auto& alloc : allocations) {
                if (alloc.function == function && alloc.variable_name == returned_var) {
                    // Variable is returned - might be transferred ownership
                    functions[function].return_paths.push_back(returned_var);
                    functions[function].returns_allocated_memory = true;
                }
            }
        }
    }

    void analyze_memory_patterns(const std::string& filename) {
        // Check for unmatched allocations
        std::unordered_map<std::string, std::vector<AllocationSite>> alloc_by_var;
        std::unordered_map<std::string, std::vector<FreeSite>> free_by_var;

        // Group by variable name
        for (const auto& alloc : allocations) {
            alloc_by_var[alloc.variable_name].push_back(alloc);
        }
        
        for (const auto& free_site : frees) {
            free_by_var[free_site.variable_name].push_back(free_site);
        }

        // Check for leaks
        for (const auto& [var_name, alloc_sites] : alloc_by_var) {
            auto free_it = free_by_var.find(var_name);
            
            if (free_it == free_by_var.end()) {
                // No corresponding free found
                for (const auto& alloc : alloc_sites) {
                    // Check if it's returned from function (potential ownership transfer)
                    bool is_returned = false;
                    for (const auto& return_var : functions[alloc.function].return_paths) {
                        if (return_var == var_name) {
                            is_returned = true;
                            break;
                        }
                    }
                    
                    if (!is_returned) {
                        errors.push_back(format_error(filename, alloc.line_number, 
                            "Potential memory leak: variable '" + var_name + 
                            "' allocated with " + alloc.allocation_type + " but never freed"));
                        leaked_variables.insert(var_name);
                    } else {
                        warnings.push_back(format_warning(filename, alloc.line_number,
                            "Variable '" + var_name + "' allocated and returned - ensure caller frees it"));
                    }
                }
            } else {
                // Check allocation/deallocation type mismatch
                for (const auto& alloc : alloc_sites) {
                    for (const auto& free_site : free_it->second) {
                        if (!is_matching_alloc_free(alloc.allocation_type, free_site.free_type)) {
                            errors.push_back(format_error(filename, free_site.line_number,
                                "Type mismatch: '" + var_name + "' allocated with " + 
                                alloc.allocation_type + " but freed with " + free_site.free_type));
                        }
                    }
                }
            }
        }

        // Check for frees without allocations
        for (const auto& [var_name, free_sites] : free_by_var) {
            if (alloc_by_var.find(var_name) == alloc_by_var.end()) {
                for (const auto& free_site : free_sites) {
                    warnings.push_back(format_warning(filename, free_site.line_number,
                        "Variable '" + var_name + "' freed but no allocation found in this file"));
                }
            }
        }
    }

    bool is_matching_alloc_free(const std::string& alloc_type, const std::string& free_type) {
        if ((alloc_type == "malloc" || alloc_type == "calloc" || alloc_type == "realloc") && 
            free_type == "free") {
            return true;
        }
        if (alloc_type == "new" && free_type == "delete") {
            return true;
        }
        if (alloc_type == "new[]" && free_type == "delete[]") {
            return true;
        }
        return false;
    }

    std::string format_error(const std::string& filename, int line, const std::string& message) {
        return "ERROR: " + filename + ":" + std::to_string(line) + ": " + message;
    }

    std::string format_warning(const std::string& filename, int line, const std::string& message) {
        return "WARNING: " + filename + ":" + std::to_string(line) + ": " + message;
    }

public:
    void print_report() {
        std::cout << "\n=== STATIC ANALYSIS REPORT ===" << std::endl;
        std::cout << "Total allocations found: " << allocations.size() << std::endl;
        std::cout << "Total deallocations found: " << frees.size() << std::endl;
        std::cout << "Potential leaks: " << leaked_variables.size() << std::endl;
        std::cout << "Errors: " << errors.size() << std::endl;
        std::cout << "Warnings: " << warnings.size() << std::endl;
        std::cout << std::endl;

        if (!errors.empty()) {
            std::cout << "=== ERRORS ===" << std::endl;
            for (const auto& error : errors) {
                std::cout << error << std::endl;
            }
            std::cout << std::endl;
        }

        if (!warnings.empty()) {
            std::cout << "=== WARNINGS ===" << std::endl;
            for (const auto& warning : warnings) {
                std::cout << warning << std::endl;
            }
            std::cout << std::endl;
        }

        if (errors.empty() && warnings.empty()) {
            std::cout << "✅ No memory leak issues detected!" << std::endl;
        }
    }

    void save_report(const std::string& output_file) {
        std::ofstream file(output_file);
        if (!file.is_open()) {
            std::cerr << "Cannot create output file: " << output_file << std::endl;
            return;
        }

        file << "# Static Memory Analysis Report\n\n";
        file << "## Summary\n";
        file << "- Total allocations found: " << allocations.size() << "\n";
        file << "- Total deallocations found: " << frees.size() << "\n";
        file << "- Potential leaks: " << leaked_variables.size() << "\n";
        file << "- Errors: " << errors.size() << "\n";
        file << "- Warnings: " << warnings.size() << "\n\n";

        if (!errors.empty()) {
            file << "## Errors\n\n";
            for (const auto& error : errors) {
                file << "- " << error << "\n";
            }
            file << "\n";
        }

        if (!warnings.empty()) {
            file << "## Warnings\n\n";
            for (const auto& warning : warnings) {
                file << "- " << warning << "\n";
            }
            file << "\n";
        }

        file << "## Allocation Details\n\n";
        for (const auto& alloc : allocations) {
            file << "- Line " << alloc.line_number << " in " << alloc.function 
                 << "(): " << alloc.variable_name << " = " << alloc.allocation_type << "()\n";
        }

        file << "\n## Deallocation Details\n\n";
        for (const auto& free_site : frees) {
            file << "- Line " << free_site.line_number << " in " << free_site.function 
                 << "(): " << free_site.free_type << "(" << free_site.variable_name << ")\n";
        }

        std::cout << "Report saved to: " << output_file << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Static Memory Leak Analyzer\n";
        std::cout << "Usage: " << argv[0] << " <file_or_directory> [output_file]\n";
        std::cout << "\nOptions:\n";
        std::cout << "  file_or_directory  Source file or directory to analyze\n";
        std::cout << "  output_file        Optional output file for report (markdown format)\n";
        std::cout << "\nExamples:\n";
        std::cout << "  " << argv[0] << " main.c\n";
        std::cout << "  " << argv[0] << " src/ report.md\n";
        return 1;
    }

    std::string target = argv[1];
    std::string output_file = (argc > 2) ? argv[2] : "";

    MemoryLeakAnalyzer analyzer;

    if (fs::is_directory(target)) {
        analyzer.analyze_directory(target);
    } else if (fs::is_regular_file(target)) {
        analyzer.analyze_file(target);
    } else {
        std::cerr << "Error: " << target << " is not a valid file or directory" << std::endl;
        return 1;
    }

    analyzer.print_report();

    if (!output_file.empty()) {
        analyzer.save_report(output_file);
    }

    return 0;
}