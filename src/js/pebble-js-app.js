/*
 * GTA2 HUD Watchface -- PebbleKit JS
 *   Key 0: KEY_THEME     0=Normal  1=ePaper  2=Monochrome
 *   Key 1: KEY_SCENE     0=Industrial  1=Downtown  2=Residential
 *   Key 2: KEY_TIME_COL  0=Yellow  1=White  2=Light Grey
 *   Key 3: KEY_DATE_COL  0=White   1=Yellow  2=Light Grey
 */
function loadCfg() {
  return {
    theme:    +(localStorage.getItem('theme')    || '0'),
    scene:    +(localStorage.getItem('scene')    || '0'),
    timecol:  +(localStorage.getItem('timecol')  || '0'),
    datecol:  +(localStorage.getItem('datecol')  || '0')
  };
}
function saveCfg(c) {
  localStorage.setItem('theme',   c.theme);
  localStorage.setItem('scene',   c.scene);
  localStorage.setItem('timecol', c.timecol);
  localStorage.setItem('datecol', c.datecol);
}
function sendMsg(c) {
  Pebble.sendAppMessage(
    { 0: c.theme, 1: c.scene, 2: c.timecol, 3: c.datecol },
    function() { console.log('ok'); },
    function(e) { console.log('fail', e); }
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
    + 'body{margin:0;font:15px/1.6 -apple-system,sans-serif;background:#0d0d0d;color:#ccc;padding:20px}'
    + 'h3{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:#555;margin:22px 0 8px}'
    + 'h3:first-child{margin-top:0}'
    + '.opt{display:flex;align-items:center;gap:12px;background:#1a1a1a;border-radius:8px;padding:13px;margin:5px 0;cursor:pointer}'
    + '.opt input{accent-color:#aaa;width:18px;height:18px;flex-shrink:0;margin:0}'
    + '.opt span{font-size:14px}'
    + '#s{display:block;width:100%;padding:14px;background:#222;color:#fff;border:1px solid #3a3a3a;border-radius:8px;font-size:15px;margin-top:24px;cursor:pointer;box-sizing:border-box}'
    + '</style></head><body>'
    + '<h3>Scene</h3>'
    + radio('scene', [
        'Industrial Sector \u2014 Map 03',
        'Downtown Sector \u2014 Map 01',
        'Residential Sector \u2014 Map 02'
      ], c.scene)
    + '<h3>Display Mode</h3>'
    + radio('theme', [
        'Normal \u2014 Full colour',
        'ePaper \u2014 Dithered greyscale',
        'Monochrome \u2014 Black & white'
      ], c.theme)
    + '<h3>Time Colour</h3>'
    + radio('timecol', [
        'Yellow \u2014 Classic GTA2',
        'White \u2014 Clean',
        'Light Grey \u2014 Subtle'
      ], c.timecol)
    + '<h3>Date Colour</h3>'
    + radio('datecol', [
        'White \u2014 Clean',
        'Yellow \u2014 Matches time',
        'Light Grey \u2014 Subtle'
      ], c.datecol)
    + '<button id="s">Save</button>'
    + '<script>'
    + 'document.getElementById("s").onclick=function(){'
    + 'function g(n){var e=document.querySelector("input[name="+n+"]:checked");return e?+e.value:0;}'
    + 'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify({'
    + 'theme:g("theme"),scene:g("scene"),timecol:g("timecol"),datecol:g("datecol")}));'
    + '};<\/script></body></html>';
  return 'data:text/html,' + encodeURIComponent(h);
}

Pebble.addEventListener('ready', function() {
  console.log('GTA2HUD ready');
  sendMsg(loadCfg());
});
Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(buildConfig(loadCfg()));
});
Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response || e.response === '' || e.response === 'CANCELLED') return;
  var raw = e.response;
  if (raw.indexOf('#') !== -1) raw = raw.substring(raw.lastIndexOf('#') + 1);
  var c;
  try { c = JSON.parse(decodeURIComponent(raw)); } catch(err) { return; }
  saveCfg(c);
  sendMsg(c);
});
