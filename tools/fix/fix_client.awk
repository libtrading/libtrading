BEGIN {
	fix_message_unparse_time_ret = 0
	fix_message_unparse_time_err = 0
	fix_message_parse_time_ret = 0
	fix_message_parse_time_err = 0
	fix_message_recv_time_ret = 0
	fix_message_recv_time_err = 0
	fix_message_send_time_ret = 0
	fix_message_send_time_err = 0
	fix_message_unparse_ret = 0
	fix_message_unparse_err = 0
	fix_message_parse_ret = 0
	fix_message_parse_err = 0
	fix_message_recv_ret = 0
	fix_message_recv_err = 0
	fix_message_send_ret = 0
	fix_message_send_err = 0
}

END {
	printf("fix_message_unparse_time_ret: %10d\n", fix_message_unparse_time_ret)
	printf("fix_message_unparse_ret: %15d\n", fix_message_unparse_ret)
	printf("fix_message_unparse_time_err: %10d\n", fix_message_unparse_time_err)
	printf("fix_message_unparse_err: %15d\n", fix_message_unparse_err)
	printf("fix_message_parse_time_ret: %12d\n", fix_message_parse_time_ret)
	printf("fix_message_parse_ret: %17d\n", fix_message_parse_ret)
	printf("fix_message_parse_time_err: %12d\n", fix_message_parse_time_err)
	printf("fix_message_parse_err: %17d\n", fix_message_parse_err)
	printf("fix_message_recv_time_ret: %13d\n", fix_message_recv_time_ret)
	printf("fix_message_recv_ret: %18d\n", fix_message_recv_ret)
	printf("fix_message_recv_time_err: %13d\n", fix_message_recv_time_err)
	printf("fix_message_recv_err: %18d\n", fix_message_recv_err)
	printf("fix_message_send_time_ret: %13d\n", fix_message_send_time_ret)
	printf("fix_message_send_ret: %18d\n", fix_message_send_ret)
	printf("fix_message_send_time_err: %13d\n", fix_message_send_time_err)
	printf("fix_message_send_err: %18d\n", fix_message_send_err)
}

$4 == "fix_message_unparse" {
	fix_message_unparse_start = $3
}

$4 == "fix_message_parse" {
	fix_message_parse_start = $3
}

$4 == "fix_message_send" {
	fix_message_send_start = $3
}

$4 == "fix_message_recv" {
	fix_message_recv_start = $3
}

$4 == "fix_message_unparse_ret" {
	fix_message_unparse_time_ret += ($3 - fix_message_unparse_start)
	fix_message_unparse_ret += 1
}

$4 == "fix_message_unparse_err" {
	fix_message_unparse_time_err += ($3 - fix_message_unparse_start)
	fix_message_unparse_err += 1
}

$4 == "fix_message_parse_ret" {
	fix_message_parse_time_ret += ($3 -fix_message_parse_start)
	fix_message_parse_ret += 1
}

$4 == "fix_message_parse_err" {
	fix_message_parse_time_err += ($3 - fix_message_parse_start)
	fix_message_parse_err += 1
}

$4 == "fix_message_recv_ret" {
	fix_message_recv_time_ret += ($3 - fix_message_recv_start)
	fix_message_recv_ret += 1
}

$4 == "fix_message_recv_err" {
	fix_message_recv_time_err += ($3 - fix_message_recv_start)
	fix_message_recv_err += 1
}

$4 == "fix_message_send_ret" {
	fix_message_send_time_ret += ($3 - fix_message_send_start)
	fix_message_send_ret += 1
}

$4 == "fix_message_send_err" {
	fix_message_send_time_err += ($3 - fix_message_send_start)
	fix_message_send_err += 1
}
