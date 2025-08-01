use crate::{AllocationInfo, MemoryStats};
use std::collections::HashMap;

pub struct MemoryTracker {
    current_stats: MemoryStats,
    peak_usage: usize,
}

impl MemoryTracker {
    pub fn new() -> Self {
        Self {
            current_stats: MemoryStats {
                total_allocated: 0,
                total_freed: 0,
                current_usage: 0,
                peak_usage: 0,
                allocation_count: 0,
                free_count: 0,
                active_allocations: HashMap::new(),
            },
            peak_usage: 0,
        }
    }

    pub fn update_stats(&mut self, new_stats: MemoryStats) {
        // Track peak usage
        if new_stats.current_usage > self.peak_usage {
            self.peak_usage = new_stats.current_usage;
        }

        // Update current stats
        self.current_stats = new_stats;
        self.current_stats.peak_usage = self.peak_usage;
    }

    pub fn get_current_stats(&self) -> &MemoryStats {
        &self.current_stats
    }

    pub fn get_final_stats(self) -> MemoryStats {
        self.current_stats
    }

    pub fn add_allocation(&mut self, address: usize, info: AllocationInfo) {
        self.current_stats.total_allocated += info.size;
        self.current_stats.current_usage += info.size;
        self.current_stats.allocation_count += 1;
        self.current_stats.active_allocations.insert(address, info);

        if self.current_stats.current_usage > self.peak_usage {
            self.peak_usage = self.current_stats.current_usage;
            self.current_stats.peak_usage = self.peak_usage;
        }
    }

    pub fn remove_allocation(&mut self, address: usize) -> Option<AllocationInfo> {
        if let Some(info) = self.current_stats.active_allocations.remove(&address) {
            self.current_stats.total_freed += info.size;
            self.current_stats.current_usage -= info.size;
            self.current_stats.free_count += 1;
            Some(info)
        } else {
            None
        }
    }

    pub fn get_allocation_info(&self, address: usize) -> Option<&AllocationInfo> {
        self.current_stats.active_allocations.get(&address)
    }

    pub fn get_allocations_by_size(&self, min_size: usize) -> Vec<(&usize, &AllocationInfo)> {
        self.current_stats
            .active_allocations
            .iter()
            .filter(|(_, info)| info.size >= min_size)
            .collect()
    }

    pub fn get_allocations_by_age(&self, min_age_seconds: i64) -> Vec<(&usize, &AllocationInfo)> {
        let now = chrono::Utc::now();
        self.current_stats
            .active_allocations
            .iter()
            .filter(|(_, info)| {
                (now - info.timestamp).num_seconds() >= min_age_seconds
            })
            .collect()
    }

    pub fn clear(&mut self) {
        self.current_stats = MemoryStats {
            total_allocated: 0,
            total_freed: 0,
            current_usage: 0,
            peak_usage: 0,
            allocation_count: 0,
            free_count: 0,
            active_allocations: HashMap::new(),
        };
        self.peak_usage = 0;
    }
}