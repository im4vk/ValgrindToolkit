use anyhow::{Context, Result};
use clap::{Arg, Command};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs::File;
use std::io::Write;
use std::time::{Duration, Instant};
use tokio::time;
use tracing::{error, info, warn};

mod memory_tracker;
mod process_monitor;
mod report_generator;

use memory_tracker::MemoryTracker;
use process_monitor::ProcessMonitor;
use report_generator::ReportGenerator;

#[derive(Debug, Serialize, Deserialize)]
pub struct AllocationInfo {
    pub size: usize,
    pub timestamp: chrono::DateTime<chrono::Utc>,
    pub stack_trace: Vec<String>,
    pub thread_id: u32,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct MemoryStats {
    pub total_allocated: usize,
    pub total_freed: usize,
    pub current_usage: usize,
    pub peak_usage: usize,
    pub allocation_count: u64,
    pub free_count: u64,
    pub active_allocations: HashMap<usize, AllocationInfo>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct ProfileReport {
    pub pid: u32,
    pub command: String,
    pub start_time: chrono::DateTime<chrono::Utc>,
    pub end_time: chrono::DateTime<chrono::Utc>,
    pub duration: Duration,
    pub memory_stats: MemoryStats,
    pub leak_summary: LeakSummary,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct LeakSummary {
    pub total_leaked_bytes: usize,
    pub leak_count: usize,
    pub largest_leak: Option<usize>,
    pub leaks_by_size: Vec<(usize, usize)>, // (size, count)
}

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt::init();

    let matches = Command::new("Rust Memory Profiler")
        .version("0.1.0")
        .about("Advanced memory leak detection and profiling tool")
        .arg(
            Arg::new("pid")
                .short('p')
                .long("pid")
                .value_name("PID")
                .help("Attach to existing process")
                .conflicts_with("command"),
        )
        .arg(
            Arg::new("command")
                .value_name("COMMAND")
                .help("Command to run and profile")
                .multiple_values(true),
        )
        .arg(
            Arg::new("output")
                .short('o')
                .long("output")
                .value_name("FILE")
                .help("Output report to file (JSON format)")
                .default_value("memory_profile.json"),
        )
        .arg(
            Arg::new("interval")
                .short('i')
                .long("interval")
                .value_name("SECONDS")
                .help("Sampling interval in seconds")
                .default_value("1"),
        )
        .arg(
            Arg::new("duration")
                .short('d')
                .long("duration")
                .value_name("SECONDS")
                .help("Maximum profiling duration in seconds")
                .default_value("60"),
        )
        .arg(
            Arg::new("live")
                .short('l')
                .long("live")
                .help("Show live memory statistics")
                .takes_value(false),
        )
        .get_matches();

    let output_file = matches.value_of("output").unwrap();
    let interval = matches
        .value_of("interval")
        .unwrap()
        .parse::<u64>()
        .context("Invalid interval")?;
    let max_duration = matches
        .value_of("duration")
        .unwrap()
        .parse::<u64>()
        .context("Invalid duration")?;
    let live_mode = matches.is_present("live");

    if let Some(pid_str) = matches.value_of("pid") {
        let pid = pid_str.parse::<u32>().context("Invalid PID")?;
        profile_existing_process(pid, output_file, interval, max_duration, live_mode).await?;
    } else if let Some(command) = matches.values_of("command") {
        let cmd_args: Vec<&str> = command.collect();
        profile_new_process(&cmd_args, output_file, interval, max_duration, live_mode).await?;
    } else {
        eprintln!("Error: Must specify either --pid or a command to run");
        std::process::exit(1);
    }

    Ok(())
}

async fn profile_existing_process(
    pid: u32,
    output_file: &str,
    interval: u64,
    max_duration: u64,
    live_mode: bool,
) -> Result<()> {
    info!("Profiling existing process PID: {}", pid);

    let monitor = ProcessMonitor::new(pid)?;
    let mut tracker = MemoryTracker::new();
    let start_time = chrono::Utc::now();
    let start_instant = Instant::now();

    let mut interval_timer = time::interval(Duration::from_secs(interval));
    let timeout = Duration::from_secs(max_duration);

    loop {
        tokio::select! {
            _ = interval_timer.tick() => {
                if let Ok(stats) = monitor.get_memory_stats().await {
                    tracker.update_stats(stats);
                    
                    if live_mode {
                        print_live_stats(&tracker.get_current_stats());
                    }
                }
                
                if start_instant.elapsed() >= timeout {
                    warn!("Maximum duration reached, stopping profiling");
                    break;
                }
            }
            _ = tokio::signal::ctrl_c() => {
                info!("Received interrupt signal, generating report...");
                break;
            }
        }

        // Check if process is still running
        if !monitor.is_running()? {
            info!("Target process has terminated");
            break;
        }
    }

    let end_time = chrono::Utc::now();
    let command = monitor.get_command_line()?;

    generate_report(
        pid,
        command,
        start_time,
        end_time,
        tracker.get_final_stats(),
        output_file,
    )
    .await?;

    Ok(())
}

async fn profile_new_process(
    cmd_args: &[&str],
    output_file: &str,
    interval: u64,
    max_duration: u64,
    live_mode: bool,
) -> Result<()> {
    info!("Starting new process: {:?}", cmd_args);

    let mut child = tokio::process::Command::new(cmd_args[0])
        .args(&cmd_args[1..])
        .spawn()
        .context("Failed to start process")?;

    let pid = child.id().context("Failed to get process ID")?;
    let monitor = ProcessMonitor::new(pid)?;
    let mut tracker = MemoryTracker::new();
    let start_time = chrono::Utc::now();
    let start_instant = Instant::now();

    let mut interval_timer = time::interval(Duration::from_secs(interval));
    let timeout = Duration::from_secs(max_duration);

    loop {
        tokio::select! {
            _ = interval_timer.tick() => {
                if let Ok(stats) = monitor.get_memory_stats().await {
                    tracker.update_stats(stats);
                    
                    if live_mode {
                        print_live_stats(&tracker.get_current_stats());
                    }
                }
                
                if start_instant.elapsed() >= timeout {
                    warn!("Maximum duration reached, terminating process");
                    let _ = child.kill().await;
                    break;
                }
            }
            result = child.wait() => {
                match result {
                    Ok(status) => {
                        info!("Process exited with status: {}", status);
                        break;
                    }
                    Err(e) => {
                        error!("Error waiting for process: {}", e);
                        break;
                    }
                }
            }
            _ = tokio::signal::ctrl_c() => {
                info!("Received interrupt signal, terminating process...");
                let _ = child.kill().await;
                break;
            }
        }
    }

    let end_time = chrono::Utc::now();
    let command = cmd_args.join(" ");

    generate_report(
        pid,
        command,
        start_time,
        end_time,
        tracker.get_final_stats(),
        output_file,
    )
    .await?;

    Ok(())
}

fn print_live_stats(stats: &MemoryStats) {
    println!("\r\x1b[2K=== Live Memory Stats ===");
    println!("Current Usage: {} KB", stats.current_usage / 1024);
    println!("Peak Usage: {} KB", stats.peak_usage / 1024);
    println!("Total Allocated: {} KB", stats.total_allocated / 1024);
    println!("Allocations: {}", stats.allocation_count);
    println!("Active Allocations: {}", stats.active_allocations.len());
    println!("========================");
}

async fn generate_report(
    pid: u32,
    command: String,
    start_time: chrono::DateTime<chrono::Utc>,
    end_time: chrono::DateTime<chrono::Utc>,
    memory_stats: MemoryStats,
    output_file: &str,
) -> Result<()> {
    let duration = end_time
        .signed_duration_since(start_time)
        .to_std()
        .unwrap_or(Duration::ZERO);

    let leak_summary = calculate_leak_summary(&memory_stats);

    let report = ProfileReport {
        pid,
        command,
        start_time,
        end_time,
        duration,
        memory_stats,
        leak_summary,
    };

    // Generate JSON report
    let json_output = serde_json::to_string_pretty(&report)?;
    let mut file = File::create(output_file)?;
    file.write_all(json_output.as_bytes())?;

    // Generate human-readable report
    let generator = ReportGenerator::new();
    generator.print_summary(&report);

    info!("Report saved to: {}", output_file);
    Ok(())
}

fn calculate_leak_summary(stats: &MemoryStats) -> LeakSummary {
    let total_leaked_bytes = stats.current_usage;
    let leak_count = stats.active_allocations.len();
    let largest_leak = stats
        .active_allocations
        .values()
        .map(|alloc| alloc.size)
        .max();

    // Group leaks by size
    let mut size_groups: HashMap<usize, usize> = HashMap::new();
    for alloc in stats.active_allocations.values() {
        *size_groups.entry(alloc.size).or_insert(0) += 1;
    }

    let mut leaks_by_size: Vec<(usize, usize)> = size_groups.into_iter().collect();
    leaks_by_size.sort_by(|a, b| b.0.cmp(&a.0)); // Sort by size, largest first

    LeakSummary {
        total_leaked_bytes,
        leak_count,
        largest_leak,
        leaks_by_size,
    }
}