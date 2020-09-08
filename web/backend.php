<?php

if (!defined('LIGHTS')) {
	http_response_code(401);
	die();
}

define('MODE_OFF', 0);
define('MODE_ON', 2);

/**
 * 
 */
class Backend
{
	private $pdo = null;
	private $remote_uri = "http://192.168.1.69";
	public $msg = null;
	public $mode = null;
	public $target = null;
	public $presets = [
		"Warm-white" => [0, 0, 0.6, 0, 0, 0],
		"Cold-white" => [0, 0.6, 0, 0, 0, 0],
	];

	function __construct() {
		$this->get_properties();
	}

	function db() {
		if ($this->pdo == null) {
			$this->pdo = new \PDO("sqlite:./db/lights.db");
			$this->pdo->exec("CREATE TABLE IF NOT EXISTS 'properties' (
					name text,
					value text
				)");
		}
		return $this->pdo;
	}

	function msg($str) {
		$this->msg = $str;
	}

	function action($action, $args = null) {
		switch ($action) {
			case "resync":
				$this->resync();
				break;
			case "on":
				$this->set_mode(MODE_ON);
				break;
			case "off":
				$this->set_mode(MODE_OFF);
				break;
			case "target":
				$this->set_target(json_decode($args));
				break;
		}
	}

	function store_property($name, $value) {
		try {
			$db = $this->db();
			$sth = $db->prepare("DELETE FROM properties WHERE name = :name");
			$sth->execute([':name' => $name]);
			$sth = $db->prepare("INSERT OR FAIL INTO properties (name, value) VALUES (:name, :value)");
			$sth->execute([':name' => $name, ':value' => json_encode($value)]);
		} catch(\PDOException $e) {
			$this->msg("DB failure: " . $e);
		}
	}

	function get_properties() {
		try {
			$db = $this->db();
			$result = $db->query("SELECT name, value FROM 'properties'");
			$rows = $result->fetchAll(PDO::FETCH_CLASS);
			foreach ($rows as $r) {
				switch ($r->name) {
					case "mode":
						$this->mode = json_decode($r->value);
						break;
					case "target":
						$this->target = json_decode($r->value);
						break;
				}
			}
		} catch(\PDOException $e) {
			$this->msg("DB failure: " . $e);
		}
	}

	function get() {
		$ch = curl_init();
		curl_setopt($ch, CURLOPT_URL, $this->remote_uri . "/get");
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
		$output = curl_exec($ch);
		curl_close($ch);
		return json_decode($output);
	}

	function set($name, $value) {
		$ch = curl_init();
		$uri = $this->remote_uri . "/set?" . $name . "=" . json_encode($value);
		curl_setopt($ch, CURLOPT_URL, $uri);
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
		curl_exec($ch);
		curl_close($ch);
	}

	function resync() {
		$output = $this->get();

		$this->mode = $output->mode;
		$this->store_property("mode", $output->mode);

		$this->target = $output->target;
		$this->store_property("target", $output->target);
	}

	function set_mode($mode) {
		$this->mode = $mode;
		$this->store_property("mode", $mode);

		$this->set("mode", $mode);
	}

	function set_target($target) {
		$this->target = $target;
		$this->store_property("target", $target);

		$this->set("target", $target);
	}
}

$backend = new Backend();

$backend->db();

if (!empty($_REQUEST['action'])) {
	if (!empty($_REQUEST['args']))
		$backend->action($_REQUEST['action'], $_REQUEST['args']);
	else
		$backend->action($_REQUEST['action']);
	http_response_code(200);
	die(json_encode($backend));
}

?>
