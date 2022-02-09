<?php

require_once('../../cdbnpp/bootstrap.php');

header('Access-Control-Allow-Origin: *');
$auth = CDBNPP\Auth::Instance();
if (
  $auth->can_set()
  && $_SERVER['REQUEST_METHOD'] == 'POST'
) {
  $data = CDBNPP\Service::Instance()->payload_set();
} else {
  header("HTTP/1.1 403 Forbidden");
  echo 'HTTP/1.1 403 Forbidden';
  exit;
}

if ( is_array($data) ) {
	if ( !empty($data['error']) ) {
	  header('HTTP/1.1 400 Bad Request');
	}
	header('Content-Type: application/json;charset=utf-8');
  echo json_encode( $data, JSON_PRETTY_PRINT | JSON_THROW_ON_ERROR | JSON_UNESCAPED_UNICODE );
} else {
	header('Content-Type: text/plain;charset=utf-8');
	echo $data;
}
exit;