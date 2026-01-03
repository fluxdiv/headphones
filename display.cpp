#include "display.hpp"

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

// length 32 = "Hash" + "Protocol" + "Src -> Dest" + "Timestamp"
// 4 spacers (left aligned)
void print_table_header(int cols) {
    // int spw = (win->ws_col - 32) / 4;
    move(2, hash_start);
    printw("Hash");
    move(2, proto_start);
    printw("Protocol");
    move(2, src_start);
    printw("Src");
    move(2, dest_start);
    printw("Dest");
    move(2, timestamp_start);
    printw("Timestamp");
    // auto out_str = fmt::format("Hash{:<{}}", ' ', spw);
    // out_str += fmt::format("Protocol{:<{}}", ' ', spw);
    // out_str += fmt::format("Src -> Dest{:<{}}", ' ', spw);
    // out_str += fmt::format("Timestamp{:<{}}", ' ', spw);
    // // flush required 
    // // std::cout << std::flush;
    // printw("%s", out_str.c_str());
}

void print_stats(int cols, unsigned long packets_total, unsigned long bytes_total) {
    move(0,0);
    // - print outer_pad spaces
    // int outer_pad = (win->ws_col - 40) / 2;
    int outer_pad = (cols - 40) / 2;
    auto out_str = fmt::format("{:>{}}", ' ', outer_pad);
    // - print "Packets: "
    out_str += fmt::format("Packets: ");
    // - print p_str, width 12, left aligned
    auto p_str = fmt::format("{}", (double)packets_total);
    if (unlikely(packets_total > 99999999999)) {
        p_str = fmt::format("{:.6g}", (double)packets_total);
    }
    out_str += fmt::format("{:<12}", p_str);
    // fmt::print("{:>{}}\n", ' ', var);    // width var
    // - print "Bytes: "
    out_str += fmt::format("Bytes: ");
    // - print b_str, width 12, left aligned
    auto b_str = fmt::format("{}", (double)bytes_total);
    if (unlikely(bytes_total > 999999999999)) {
        b_str = fmt::format("{:.6g}", (double)bytes_total);
    }
    out_str += fmt::format("{:<12}", b_str);
    // - print outer_pad spaces (or dont..)
    // flush required 
    // std::cout << std::flush;
    printw("%s", out_str.c_str());
}

