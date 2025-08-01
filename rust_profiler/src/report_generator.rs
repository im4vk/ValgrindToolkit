use crate::ProfileReport;
use prettytable::{Cell, Row, Table};
use std::fmt::Write;

pub struct ReportGenerator;

impl ReportGenerator {
    pub fn new() -> Self {
        Self
    }

    pub fn print_summary(&self, report: &ProfileReport) {
        println!("\n=== MEMORY PROFILE REPORT ===");
        println!("Process: {} (PID: {})", report.command, report.pid);
        println!("Duration: {:.2?}", report.duration);
        println!("Profiling Period: {} to {}", report.start_time, report.end_time);
        println!();

        self.print_memory_statistics(report);
        self.print_leak_analysis(report);
        self.print_allocation_details(report);
    }

    fn print_memory_statistics(&self, report: &ProfileReport) {
        let stats = &report.memory_stats;
        
        println!("=== MEMORY STATISTICS ===");
        let mut table = Table::new();
        table.add_row(Row::new(vec![
            Cell::new("Metric"),
            Cell::new("Value"),
            Cell::new("Human Readable"),
        ]));

        table.add_row(Row::new(vec![
            Cell::new("Total Allocated"),
            Cell::new(&stats.total_allocated.to_string()),
            Cell::new(&Self::format_bytes(stats.total_allocated)),
        ]));

        table.add_row(Row::new(vec![
            Cell::new("Total Freed"),
            Cell::new(&stats.total_freed.to_string()),
            Cell::new(&Self::format_bytes(stats.total_freed)),
        ]));

        table.add_row(Row::new(vec![
            Cell::new("Current Usage"),
            Cell::new(&stats.current_usage.to_string()),
            Cell::new(&Self::format_bytes(stats.current_usage)),
        ]));

        table.add_row(Row::new(vec![
            Cell::new("Peak Usage"),
            Cell::new(&stats.peak_usage.to_string()),
            Cell::new(&Self::format_bytes(stats.peak_usage)),
        ]));

        table.add_row(Row::new(vec![
            Cell::new("Allocations"),
            Cell::new(&stats.allocation_count.to_string()),
            Cell::new("-"),
        ]));

        table.add_row(Row::new(vec![
            Cell::new("Frees"),
            Cell::new(&stats.free_count.to_string()),
            Cell::new("-"),
        ]));

        table.add_row(Row::new(vec![
            Cell::new("Active Allocations"),
            Cell::new(&stats.active_allocations.len().to_string()),
            Cell::new("-"),
        ]));

        table.printstd();
        println!();
    }

    fn print_leak_analysis(&self, report: &ProfileReport) {
        let leak_summary = &report.leak_summary;
        
        println!("=== LEAK ANALYSIS ===");
        
        if leak_summary.leak_count == 0 {
            println!("🎉 No memory leaks detected!");
            return;
        }

        println!("⚠️  Memory leaks detected!");
        println!("Total leaked: {}", Self::format_bytes(leak_summary.total_leaked_bytes));
        println!("Number of leaks: {}", leak_summary.leak_count);
        
        if let Some(largest) = leak_summary.largest_leak {
            println!("Largest leak: {}", Self::format_bytes(largest));
        }

        println!("\nLeaks by size:");
        let mut table = Table::new();
        table.add_row(Row::new(vec![
            Cell::new("Size"),
            Cell::new("Count"),
            Cell::new("Total"),
        ]));

        for (size, count) in &leak_summary.leaks_by_size {
            table.add_row(Row::new(vec![
                Cell::new(&Self::format_bytes(*size)),
                Cell::new(&count.to_string()),
                Cell::new(&Self::format_bytes(size * count)),
            ]));
        }

        table.printstd();
        println!();
    }

    fn print_allocation_details(&self, report: &ProfileReport) {
        let stats = &report.memory_stats;
        
        if stats.active_allocations.is_empty() {
            return;
        }

        println!("=== ACTIVE ALLOCATIONS ===");
        
        // Show top 10 largest allocations
        let mut allocations: Vec<_> = stats.active_allocations.iter().collect();
        allocations.sort_by(|a, b| b.1.size.cmp(&a.1.size));
        
        let mut table = Table::new();
        table.add_row(Row::new(vec![
            Cell::new("Address"),
            Cell::new("Size"),
            Cell::new("Age"),
            Cell::new("Thread"),
        ]));

        for (addr, info) in allocations.iter().take(10) {
            let age = chrono::Utc::now()
                .signed_duration_since(info.timestamp)
                .num_seconds();
            
            table.add_row(Row::new(vec![
                Cell::new(&format!("0x{:x}", addr)),
                Cell::new(&Self::format_bytes(info.size)),
                Cell::new(&format!("{}s", age)),
                Cell::new(&info.thread_id.to_string()),
            ]));
        }

        table.printstd();
        
        if allocations.len() > 10 {
            println!("... and {} more allocations", allocations.len() - 10);
        }
        println!();
    }

    pub fn generate_detailed_report(&self, report: &ProfileReport) -> String {
        let mut output = String::new();
        
        writeln!(output, "# Memory Profile Report").unwrap();
        writeln!(output, "").unwrap();
        writeln!(output, "**Process:** {} (PID: {})", report.command, report.pid).unwrap();
        writeln!(output, "**Duration:** {:.2?}", report.duration).unwrap();
        writeln!(output, "**Start Time:** {}", report.start_time).unwrap();
        writeln!(output, "**End Time:** {}", report.end_time).unwrap();
        writeln!(output, "").unwrap();

        // Memory Statistics
        writeln!(output, "## Memory Statistics").unwrap();
        writeln!(output, "").unwrap();
        let stats = &report.memory_stats;
        writeln!(output, "- **Total Allocated:** {}", Self::format_bytes(stats.total_allocated)).unwrap();
        writeln!(output, "- **Total Freed:** {}", Self::format_bytes(stats.total_freed)).unwrap();
        writeln!(output, "- **Current Usage:** {}", Self::format_bytes(stats.current_usage)).unwrap();
        writeln!(output, "- **Peak Usage:** {}", Self::format_bytes(stats.peak_usage)).unwrap();
        writeln!(output, "- **Allocation Count:** {}", stats.allocation_count).unwrap();
        writeln!(output, "- **Free Count:** {}", stats.free_count).unwrap();
        writeln!(output, "").unwrap();

        // Leak Analysis
        writeln!(output, "## Leak Analysis").unwrap();
        writeln!(output, "").unwrap();
        let leak_summary = &report.leak_summary;
        
        if leak_summary.leak_count == 0 {
            writeln!(output, "✅ No memory leaks detected!").unwrap();
        } else {
            writeln!(output, "⚠️ **{} memory leaks detected**", leak_summary.leak_count).unwrap();
            writeln!(output, "").unwrap();
            writeln!(output, "- **Total Leaked:** {}", Self::format_bytes(leak_summary.total_leaked_bytes)).unwrap();
            
            if let Some(largest) = leak_summary.largest_leak {
                writeln!(output, "- **Largest Leak:** {}", Self::format_bytes(largest)).unwrap();
            }
            
            writeln!(output, "").unwrap();
            writeln!(output, "### Leaks by Size").unwrap();
            writeln!(output, "").unwrap();
            writeln!(output, "| Size | Count | Total |").unwrap();
            writeln!(output, "|------|-------|-------|").unwrap();
            
            for (size, count) in &leak_summary.leaks_by_size {
                writeln!(output, "| {} | {} | {} |", 
                    Self::format_bytes(*size), 
                    count, 
                    Self::format_bytes(size * count)
                ).unwrap();
            }
        }

        output
    }

    fn format_bytes(bytes: usize) -> String {
        const UNITS: &[&str] = &["B", "KB", "MB", "GB", "TB"];
        let mut size = bytes as f64;
        let mut unit_idx = 0;

        while size >= 1024.0 && unit_idx < UNITS.len() - 1 {
            size /= 1024.0;
            unit_idx += 1;
        }

        if unit_idx == 0 {
            format!("{} {}", bytes, UNITS[unit_idx])
        } else {
            format!("{:.2} {}", size, UNITS[unit_idx])
        }
    }
}