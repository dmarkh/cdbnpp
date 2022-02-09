<?php

namespace CDBNPP;

function uuidv4() {
	$hex = bin2hex(random_bytes(16));
	$time_low = substr($hex, 0, 8);
	$time_mid = substr($hex, 8, 4);
	$time_hi_and_version = '4' . substr($hex, 13, 3);
	$clock_seq_hi_and_reserved = base_convert(substr($hex, 16, 2), 16, 10);
	$clock_seq_hi_and_reserved &= 0b00111111;
	$clock_seq_hi_and_reserved |= 0b10000000;
	$clock_seq_low = substr($hex, 18, 2);
	$node = substr($hex, 20);
	$uuid = sprintf('%s-%s-%s-%02x%s-%s',
		$time_low, $time_mid, $time_hi_and_version,
		$clock_seq_hi_and_reserved, $clock_seq_low,
		$node
	);
	return $uuid;
}
