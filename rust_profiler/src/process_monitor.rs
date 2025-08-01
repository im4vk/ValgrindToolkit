use crate::MemoryStats;
use anyhow::{Context, Result};
use procfs::process::Process;
use std::collections::HashMap;

pub struct ProcessMonitor {
    pid: u32,
    process: Process,
}

impl ProcessMonitor {
    pub fn new(pid: u32) -> Result<Self> {
        let process = Process::new(pid as i32)
            .context(format!("Failed to attach to process {}", pid))?;

        Ok(Self { pid, process })
    }

    pub async fn get_memory_stats(&self) -> Result<MemoryStats> {
        let stat = self.process.stat().context("Failed to read process stat")?;
        let statm = self.process.statm().context("Failed to read process statm")?;
        let status = self.process.status().context("Failed to read process status")?;

        // Calculate memory usage from /proc/pid/statm
        let page_size = procfs::page_size();
        let current_usage = (statm.resident * page_size) as usize;

        // Try to get more detailed memory info from /proc/pid/status
        let vmrss = status
            .vmrss
            .map(|kb| kb * 1024)
            .unwrap_or(current_usage);
        let vmpeak = status
            .vmpeak
            .map(|kb| kb * 1024)
            .unwrap_or(current_usage);

        // For this simplified version, we'll use the RSS as current usage
        // In a real implementation, you'd need to hook into malloc/free or use ptrace
        let stats = MemoryStats {
            total_allocated: vmpeak,
            total_freed: vmpeak.saturating_sub(vmrss),
            current_usage: vmrss,
            peak_usage: vmpeak,
            allocation_count: 0, // Would need to track this separately
            free_count: 0,       // Would need to track this separately
            active_allocations: HashMap::new(), // Would need malloc/free hooking
        };

        Ok(stats)
    }

    pub fn is_running(&self) -> Result<bool> {
        match self.process.stat() {
            Ok(_) => Ok(true),
            Err(_) => Ok(false),
        }
    }

    pub fn get_command_line(&self) -> Result<String> {
        let cmdline = self.process.cmdline().context("Failed to read cmdline")?;
        Ok(cmdline.join(" "))
    }

    pub fn get_pid(&self) -> u32 {
        self.pid
    }

    pub async fn get_memory_maps(&self) -> Result<Vec<MemoryMapping>> {
        let maps = self.process.maps().context("Failed to read memory maps")?;
        let mut mappings = Vec::new();

        for map in maps {
            mappings.push(MemoryMapping {
                start_address: map.address.0,
                end_address: map.address.1,
                size: map.address.1 - map.address.0,
                permissions: format!("{:?}", map.perms),
                pathname: map.pathname.map(|p| format!("{:?}", p)),
            });
        }

        Ok(mappings)
    }

    pub async fn get_open_files(&self) -> Result<Vec<String>> {
        let fd_dir = self.process.fd().context("Failed to read file descriptors")?;
        let mut files = Vec::new();

        for fd_entry in fd_dir {
            if let Ok(fd) = fd_entry {
                if let Ok(target) = fd.target() {
                    files.push(format!("{}: {:?}", fd.fd, target));
                }
            }
        }

        Ok(files)
    }
}

#[derive(Debug, Clone)]
pub struct MemoryMapping {
    pub start_address: u64,
    pub end_address: u64,
    pub size: u64,
    pub permissions: String,
    pub pathname: Option<String>,
}