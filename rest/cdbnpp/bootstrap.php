<?php

if ( 0 ) {
	// production: do not display errors, log them
  ini_set('display_errors', 0);
  ini_set('log_errors', 1);
} else {
	// debug: dislay all errors
  ini_set('display_errors', 1);
}

function sanitize_alnum( $str ) {
	return preg_replace( "/[^a-zA-Z0-9]+/", "", $str );
}

function sanitize_alnumdash( $str ) {
	return preg_replace( "/[^a-zA-Z0-9\-]+/", "", $str );
}

function sanitize_alnumscore( $str ) {
	return preg_replace( "/[^a-zA-Z0-9\_]+/", "", $str );
}

// redirect to HTTPS, always
if ( $_SERVER['REQUEST_SCHEME'] == 'http' || $_SERVER['HTTPS'] == 'off' ) {
	$location = 'https://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'];
	header('Location: ' . $location);
	exit;
}

// include all required codes
$root = dirname( __FILE__ );
require_once( $root . '/config.php' );
require_once( $root . '/jwt_service.php' );
require_once( $root . '/auth.php' );
require_once( $root . '/uuidv4.php' );

// check auth
$auth = CDBNPP\Auth::Instance();

require_once( $root . '/service.php' );
