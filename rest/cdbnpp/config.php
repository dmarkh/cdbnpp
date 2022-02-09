<?php

namespace CDBNPP;

class Config {
	var $cfg = NULL;
	function __construct() {
		$this->init();
	}
	function &Instance () {
		static $instance;
		if (!isset($instance)) { $c = __CLASS__; $instance = new $c; }
		return $instance;
	}
	function init() {
		$path = dirname(__FILE__) . '/settings.php';
		require_once( $path );
		if ( function_exists( 'cdbnpp_settings' ) ) {
			$this->cfg = cdbnpp_settings();
		} else {
			echo 'function does not exist'; exit;
			$this->cfg = array();
		}
	}
	function get($section, $param = NULL) {
		if ( !empty($param) ) {
			return $this->cfg[$section][$param];
		}
		return $this->cfg[$section];
	}
	function list_all() { echo '<pre>'; print_r($this->cfg); echo '</pre>'; }
}
