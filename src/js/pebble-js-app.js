/*
 * GTA2Watch -- PebbleKit JS
 * Keys (alphabetical = index):
 *   Key 0: A_SHOW_DATE    0=hide  1=show
 *   Key 1: B_SHOW_MENU    0=hide  1=show
 *   Key 2: C_SHOW_STEPS   0=battery  1=steps
 *   Key 3: D_TIME_LAST    0=red  1=gold
 */

function loadCfg() {
  return {
    sd: +(localStorage.getItem('sd') || '1'),
    sm: +(localStorage.getItem('sm') || '1'),
    ss: +(localStorage.getItem('ss') || '0'),
    tl: +(localStorage.getItem('tl') || '0')
  };
}

function saveCfg(c) {
  localStorage.setItem('sd', c.sd);
  localStorage.setItem('sm', c.sm);
  localStorage.setItem('ss', c.ss);
  localStorage.setItem('tl', c.tl);
}

function sendMsg(c) {
  Pebble.sendAppMessage(
    { 0: c.sd, 1: c.sm, 2: c.ss, 3: c.tl },
    function() { console.log('GTA2Watch: settings sent ok'); },
    function(e) { console.log('GTA2Watch: send failed', JSON.stringify(e)); }
  );
}

function buildConfig(c) {
  function radio(name, opts, sel) {
    return opts.map(function(l, i) {
      return '<label class="opt"><input type="radio" name="' + name
        + '" value="' + i + '"' + (i === sel ? ' checked' : '')
        + '><span>' + l + '</span></label>';
    }).join('');
  }

  var h = '<!DOCTYPE html><html><head>'
    + '<meta charset="utf-8">'
    + '<meta name="viewport" content="width=device-width,initial-scale=1">'
    + '<style>'
    + 'body{margin:0;font:15px/1.6 -apple-system,sans-serif;background:#000;color:#ccc;padding:20px}'
    + 'h3{font-size:11px;text-transform:uppercase;letter-spacing:.1em;color:#cc0000;margin:22px 0 8px}'
    + 'h3:first-child{margin-top:0}'
    + '.opt{display:flex;align-items:center;gap:12px;background:#111;border-radius:8px;padding:13px;margin:5px 0;border:1px solid #1a1a1a}'
    + '.opt input{accent-color:#cc0000;width:18px;height:18px;flex-shrink:0;margin:0}'
    + '.opt span{font-size:14px;color:#ddd}'
    + '#s{display:block;width:100%;padding:14px;background:#cc0000;color:#fff;border:none;border-radius:8px;font-size:15px;font-weight:bold;margin-top:24px;cursor:pointer;box-sizing:border-box;letter-spacing:1px}'
    + '</style></head><body>'
    + '<h3>Date Bar</h3>'
    + radio('sd', ['Hide', 'Show'], c.sd)
    + '<h3>Menu Lines</h3>'
    + radio('sm', ['Hide', 'Show'], c.sm)
    + '<h3>Stat Line 4</h3>'
    + radio('ss', ['Battery %', 'Steps count'], c.ss)
    + '<h3>Accent Color</h3>'
    + radio('tl', ['Red', 'Gold'], c.tl)
    + '<button id="s">SAVE GAME</button>'
    + '<script>'
    + 'document.getElementById("s").onclick=function(){'
    + 'function g(n){var e=document.querySelector("input[name="+n+"]:checked");return e?+e.value:0;}'
    + 'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify({'
    + 'sd:g("sd"),sm:g("sm"),ss:g("ss"),tl:g("tl")}));'
    + '};<\/script></body></html>';

  return 'data:text/html,' + encodeURIComponent(h);
}

/* ready fires first — safe to register other listeners here */
Pebble.addEventListener('ready', function() {
  console.log('GTA2Watch: ready');
});

Pebble.addEventListener('showConfiguration', function() {
  var cfg = loadCfg();
  Pebble.openURL(buildConfig(cfg));
});

Pebble.addEventListener('webviewclosed', function(e) {
  /* Guard against null/empty/cancelled */
  if (!e || !e.response || e.response === '' || e.response === 'CANCELLED') {
    console.log('GTA2Watch: config cancelled');
    return;
  }
  var raw = e.response;
  /* Strip everything before the last # */
  if (raw.indexOf('#') !== -1) {
    raw = raw.substring(raw.lastIndexOf('#') + 1);
  }
  if (!raw || raw === '') return;
  var c;
  try {
    c = JSON.parse(decodeURIComponent(raw));
  } catch (err) {
    console.log('GTA2Watch: parse error', err);
    return;
  }
  if (!c) return;
  saveCfg(c);
  sendMsg(c);
});
