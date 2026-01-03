#pragma once

#include <fmt/core.h>
#include <ncurses.h>

//---------------------------------------------------------
// Formatting constants
// Not great but resizing is a want not need 
static const int hash_start = 0;
static const int proto_start = 11;
static const int src_start = 42;
static const int dest_start = 61;
static const int timestamp_start = 80;

void print_table_header(int cols);
void print_stats(int cols, unsigned long packets_total, unsigned long bytes_total);
