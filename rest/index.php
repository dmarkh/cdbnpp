<?php
	require_once('./cdbnpp/bootstrap.php');
?>
<!DOCTYPE html>
<html>
<head>
	<title>CDBNPP HTTPS SERVICE</title>
	<style>
		html, body {
			font-family: mono, verdana, helvetica, arial;
			font-size: 100%;
			background-color: #000;
			color: #CCC;
		}
		a:link, a:active, a:visited {
			text-decoration: none;
			color: #FFF;
		}
	</style>
</head>
<body style="">

<h1>CDBNPP HTTPS SERVICE</h1>

<h2>CDB PUBLIC API [role:get]:</h2>
<ul>
	<li>HTTPS GET <a href="api/tags/">/api/tags/</a> - get all tags</h3>
	<li>HTTPS GET <a href="api/schema/">/api/schema/</a> - get specific JSON schema for validation purposes</h3>
	<li>HTTPS POST <a href="api/payloadrefs/">/api/payloadrefs/</a> - bulk get payload refs by posting an array of [ path,et,met,run.seq ]</h3>
	<li>HTTPS GET <a href="api/payloadget/">/api/payloadget/</a> - get db-embedded payload file</h3>
</ul>

<h2>CDB ADMIN API [role:write,create,delete]:</h2>
<ul>
	<li>HTTPS POST <a href="api/createtag/">/api/createtag/</a> - create new tag / struct</li>
	<li>HTTPS POST <a href="api/payloadset/">/api/payloadset/</a></li>
</ul>

<h2>CDB TOOLS</h2>
<ul>
	<li>Tag Browser - TBD</li>
</ul>

<h2>CONTACTS</h2>
<ul>
	<li><?php echo CDBNPP\Config::Instance()->get('settings', 'contact_person'); ?></li>
</ul>
</body>
</html>