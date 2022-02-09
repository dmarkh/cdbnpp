<?php

namespace CDBNPP;

// restore function if dropped by FCGI
if (!function_exists('getallheaders')) {
  function getallheaders() {
    $headers = [];
    foreach ($_SERVER as $name => $value) {
      if (substr($name, 0, 5) == 'HTTP_') {
        $headers[str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($name, 5)))))] = $value;
      }
    }
    return $headers;
  }
}

class Auth {

	private $token = null;
	private $user = null;
	private $permissions = [
		'get' => false,
		'set' => false,
		'admin' => false
	];

  public function __construct() {
    $this->init();
  }

  public function &Instance () {
    static $instance;
    if (!isset($instance)) { $c = __CLASS__; $instance = new $c; }
    return $instance;
  }

  private function init() {
		// check JWT token, assign permissions accordingly

		if ( empty($_SERVER['HTTP_AUTHORIZATION']) ) { return false; }
		list( $bearer, $this->token ) = explode( ' ', $_SERVER['HTTP_AUTHORIZATION'] );
		if ( empty( $this->token ) ) { return false; }

		// decode JWT token
		$jwt = JWTService::Instance();
		if ( $jwt->verify( $this->token ) == false ) {
			return;
		}

		$data = $jwt->decode( $this->token );
		$this->user = $data['payload']['iss'];

		$users = Config::Instance()->get('auth','users');
		$acc = !empty( $users[$this->user]['access'] ) ? $users[$this->user]['access'] : '';

		switch($acc) {
			case 'get':
				$this->permissions['get'] = true;
				break;
			case 'set':
				$this->permissions['get'] = true;
				$this->permissions['set'] = true;
				break;
			case 'admin':
				$this->permissions['get'] = true;
				$this->permissions['set'] = true;
				$this->permissions['admin'] = true;
				break;
		}

	}

	public function is_authenticated() {
		return !empty( $this->user );
	}

	function can_get() {
		return $this->is_authenticated() && $this->permissions['get'];
	}

	function can_set() {
		return $this->is_authenticated() && $this->permissions['set'];
	}

	function can_admin() {
		return $this->is_authenticated() && $this->permissions['admin'];
	}

}