<?php

namespace CDBNPP;

class JWTService {

	public function __construct() {
	}

  function &Instance () {
    static $instance;
    if (!isset($instance)) { $c = __CLASS__; $instance = new $c; }
    return $instance;
  }

	public static function print( $token ) {
		$data = explode( '.', $token );
		return 'header: ' .base64_decode($data[0]).', payload: '.base64_decode($data[1]).', signature: '.$data[2];
	}

	public function decode( $token ) {
		$data = explode( '.', $token );
		return  [
			'header' => base64_decode( $data[0] ),
			'payload' => json_decode( base64_decode( $data[1] ), true ),
			'signature' => $data[2]
		];
	}

	public function verify( $token ) {

		$decoded = $this->decode($token);

		if ( intval($decoded['payload']['iat']) > time() ) {
			error_log('token issue time is in future', 0 );
			return false; // issue time is in the future
		}
		if ( intval($decoded['payload']['iat'] >= intval($decoded['payload']['exp']) ) ) {
			error_log('token issue time is past expiration time', 0 );
			return false; // issue time is past expiration time
		}
		if ( intval($decoded['payload']['exp']) < time() ) {
			error_log('token expired', 0 );
			return false;
		}

		$users = Config::Instance()->get('auth','users');
		$user = $decoded['payload']['iss'];

		if ( empty($user) || empty( $users[ $user ] ) ) {
			error_log('unknown user: ' .$user, 0 );
			return false; // unknown user
		}

		// ok, basic checks done, now let's verify token signature
		$pass = $users[ $user ]['pass'];

		$data = explode( '.', $token );
		$signature = hash_hmac( 'sha256', $data[0] . '.' . $data[1], $pass, true );
		$base64UrlSignature = str_replace(['+', '/', '='], ['-', '_', ''], base64_encode($signature));

		if ( $base64UrlSignature != $data[2] ) {
			error_log( 'token denied, signature does not match' );
			return false; // signature is forged
		}

		return $users[ $user ]['access'];
	}

/*
	public function generate( $user = 'anyone', $op = 'read', $valid_for = 365*24*60*60 ) {
		if ( empty($this->secret) ) { return ''; }
		$header = json_encode(['typ' => 'JWT', 'alg' => 'HS256']);
		$payload = json_encode([ 'user' => $username, 'op' => $op, 'cre' => time(), 'exp' => time() + $valid_for ]);
		$base64UrlHeader = str_replace(['+', '/', '='], ['-', '_', ''], base64_encode($header));
		$base64UrlPayload = str_replace(['+', '/', '='], ['-', '_', ''], base64_encode($payload));
		$signature = hash_hmac('sha256', $base64UrlHeader . '.' . $base64UrlPayload, $this->secret, true);
		$base64UrlSignature = str_replace(['+', '/', '='], ['-', '_', ''], base64_encode($signature));
		$jwt = $base64UrlHeader . "." . $base64UrlPayload . "." . $base64UrlSignature;
		return $jwt;
	}
*/

	private $secret;

}
