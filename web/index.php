<?php

error_reporting(E_ALL);

if (!preg_match('/192\.168\.1\.[0-9]+/', $_SERVER['REMOTE_ADDR'])) {
  http_response_code(401);
  die();
}

define('LIGHTS', true);
include_once "backend.php";

?>
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="theme-color" content="#ef4e2a">

  <title>Lights</title>

  <link rel="manifest" href="manifest.json">
  <link rel="stylesheet" href="https://unpkg.com/onsenui@2.10.10/css/onsenui.css" integrity="sha256-bCd32CIrUXHWVebLB/bKGTevHlOkOyFifnnZqqqjsoc=" crossorigin="anonymous" />
  <link rel="stylesheet" href="https://unpkg.com/onsenui@2.10.10/css/dark-onsen-css-components.min.css" integrity="sha256-vgHQ5eWafCSyXa3wQc90Kcichb72z+K3kXNWsF9H+Nk=" crossorigin="anonymous" />
</head>
<body>

<ons-page>
  <section style="padding: 8px">
    <ons-button modifier="large" class="action" data-action="on">On</ons-button>
    <p />
    <ons-button modifier="large" class="action" data-action="off">Off</ons-button>
  </section>
  <section style="padding: 8px">
    <ons-list id="presets">
      <ons-list-header>Presets</ons-list-header>
    </ons-list>
  </section>

<!--
  <section style="padding: 8px">
    <ons-button class="action" data-action="resync">Resync</ons-button>
    <p />
    <textarea id="msg" cols=40 rows=20></textarea><br>
  </section>
-->
</ons-page>

<script src="//unpkg.com/onsenui@2.10.10/js/onsenui.min.js" integrity="sha256-UVdD74iH0rWvjKQPhwgFzzKWnjOcf+uJ8v//EMAgOGk=" crossorigin="anonymous"></script>
<script src="//code.jquery.com/jquery-3.5.1.min.js" integrity="sha256-9/aliU8dGd2tb6OSsuzixeV4y/faTqgFtohetphbbj0=" crossorigin="anonymous"></script>
<script src="//cdnjs.cloudflare.com/ajax/libs/chroma-js/2.1.0/chroma.min.js" integrity="sha512-yocoLferfPbcwpCMr8v/B0AB4SWpJlouBwgE0D3ZHaiP1nuu5djZclFEIj9znuqghaZ3tdCMRrreLoM8km+jIQ==" crossorigin="anonymous"></script>
<script src="frontend.js"></script>
<script type="text/javascript">
backend = <?=json_encode($backend)?>;
if (window.addEventListener) window.addEventListener("load", ready, false);
else if (window.attachEvent) window.attachEvent("onload", ready);
else window.onload = ready;
</script>
</body>
</html>
