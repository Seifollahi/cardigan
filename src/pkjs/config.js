/*
 * Settings page, shipped as a data: URI so it needs no hosting and
 * works offline. Card list is injected via the URL; the page returns
 * the edited list through pebblejs://close#<json>.
 */
'use strict';

var PAGE = '<!DOCTYPE html><html><head><meta charset="utf-8">' +
'<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">' +
'<title>Cardigan</title><style>' +
'*{box-sizing:border-box;margin:0;-webkit-tap-highlight-color:transparent}' +
'body{font-family:-apple-system,Roboto,sans-serif;background:#111319;color:#eceef2;padding:14px 12px 90px}' +
'h1{font-size:19px;margin:6px 2px 2px}' +
'.sub{font-size:12px;color:#8b90a0;margin:0 2px 14px}' +
'.card{background:#1b1e27;border:1px solid #2a2e3b;border-radius:14px;padding:12px;margin-bottom:10px}' +
'.row{display:flex;gap:8px;align-items:center}' +
'.chip{width:34px;height:34px;border-radius:9px;flex-shrink:0;display:flex;align-items:center;justify-content:center;font-weight:800;color:#fff;font-size:14px}' +
'.meta{flex:1;min-width:0}.nm{font-weight:700;font-size:14px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}' +
'.cd{font-size:11px;color:#8b90a0;margin-top:2px}' +
'button{border:none;border-radius:9px;font-family:inherit;cursor:pointer}' +
'.ib{width:32px;height:32px;background:#262a36;color:#aeb3c2;font-size:14px;margin-left:4px}' +
'.ib.on{color:#ffb020}.ib.del{color:#ff6b6b}' +
'.form{margin-top:10px;display:none}.form.open{display:block}' +
'label{display:block;font-size:10px;font-weight:700;letter-spacing:.1em;color:#8b90a0;text-transform:uppercase;margin:10px 0 4px}' +
'input,select{width:100%;background:#12141b;border:1px solid #2a2e3b;border-radius:9px;color:#eceef2;padding:10px;font-size:14px;font-family:inherit;outline:none}' +
'input:focus{border-color:#ff4d29}' +
'.pal{display:grid;grid-template-columns:repeat(8,1fr);gap:6px}' +
'.sw{aspect-ratio:1;border-radius:8px;border:2px solid transparent}.sw.on{border-color:#fff}' +
'.grid2{display:flex;gap:8px}.grid2>div{flex:1}' +
'.add{width:100%;padding:13px;background:#262a36;color:#eceef2;font-weight:700;font-size:14px;border-radius:12px;border:1px dashed #3a3f4f;margin-top:2px}' +
'.savebar{position:fixed;left:0;right:0;bottom:0;padding:12px;background:linear-gradient(transparent,#111319 30%)}' +
'.save{width:100%;padding:15px;background:#ff4d29;color:#fff;font-weight:800;font-size:15px;border-radius:13px}' +
'.limit{font-size:11px;color:#8b90a0;text-align:center;margin-top:8px}' +
'</style></head><body>' +
'<h1>Cardigan</h1><p class="sub">The wallet you wear. Cards sync to the watch when you save — digits-only codes scan best.</p>' +
'<div id="list"></div>' +
'<button class="add" onclick="addCard()">+ Add card</button>' +
'<div class="savebar"><button class="save" onclick="save()">Save &amp; sync to watch</button>' +
'<div class="limit" id="limit"></div></div>' +
'<script>' +
'var PAL=["#00AA55","#0055AA","#AA0055","#FF5500","#5500AA","#00AAAA","#AA5500","#AA0000",' +
'"#55AA00","#0000AA","#AA00AA","#005555","#FF0055","#555555","#000000","#FFAA00"];' +
'var MAX=12;var cards=[];var open=-1;' +
'try{cards=JSON.parse(decodeURIComponent(location.hash.slice(1)))||[]}catch(e){cards=[]}' +
'function esc(s){return String(s).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/"/g,"&quot;")}' +
'function render(){var h="";for(var i=0;i<cards.length;i++){var c=cards[i];' +
'h+="<div class=card><div class=row>"+' +
'"<div class=chip style=background:"+c.color+">"+esc((c.name||"?")[0].toUpperCase())+"</div>"+' +
'"<div class=meta><div class=nm>"+esc(c.name||"Card")+"</div><div class=cd>"+esc(c.code||"")+" \\u00b7 "+(c.type||"CODE128")+"</div></div>"+' +
'"<button class=\\"ib"+(c.fav?" on":"")+"\\" onclick=fav("+i+")>\\u2605</button>"+' +
'"<button class=ib onclick=move("+i+",-1)>\\u2191</button>"+' +
'"<button class=ib onclick=move("+i+",1)>\\u2193</button>"+' +
'"<button class=\\"ib del\\" onclick=del("+i+")>\\u2715</button></div>"+' +
'"<div class=\\"form"+(open===i?" open":"")+"\\">"+' +
'"<label>Name</label><input value=\\""+esc(c.name||"")+"\\" maxlength=23 onchange=upd("+i+",\\"name\\",this.value)>"+' +
'"<div class=grid2><div><label>Code</label><input value=\\""+esc(c.code||"")+"\\" maxlength=26 onchange=upd("+i+",\\"code\\",this.value)></div>"+' +
'"<div><label>Format</label><select onchange=upd("+i+",\\"type\\",this.value)>"+' +
'"<option"+(c.type!=="QR"?" selected":"")+">CODE128</option><option"+(c.type==="QR"?" selected":"")+">QR</option></select></div></div>"+' +
'"<div class=grid2><div><label>Points</label><input type=number value=\\""+(c.points||0)+"\\" onchange=upd("+i+",\\"points\\",parseInt(this.value)||0)></div><div></div></div>"+' +
'"<label>Colour (Pebble 64-colour palette)</label><div class=pal>"+PAL.map(function(p){' +
'return "<div class=\\"sw"+(p===c.color?" on":"")+"\\" style=background:"+p+" onclick=upd("+i+",\\"color\\",\\""+p+"\\")></div>"}).join("")+"</div>"+' +
'"</div></div>"}' +
'document.getElementById("list").innerHTML=h;' +
'document.getElementById("limit").textContent=cards.length+" / "+MAX+" cards (watch storage limit)";' +
'var rows=document.querySelectorAll(".card .row .meta");' +
'for(var r=0;r<rows.length;r++)(function(idx){rows[idx].onclick=function(){open=open===idx?-1:idx;render()}})(r)}' +
'function upd(i,k,v){cards[i][k]=v;render()}' +
'function fav(i){cards[i].fav=!cards[i].fav;render()}' +
'function del(i){cards.splice(i,1);if(open===i)open=-1;render()}' +
'function move(i,d){var j=i+d;if(j<0||j>=cards.length)return;var t=cards[i];cards[i]=cards[j];cards[j]=t;render()}' +
'function addCard(){if(cards.length>=MAX)return;cards.push({name:"New card",code:"",type:"CODE128",color:PAL[cards.length%PAL.length],fav:false,points:0});open=cards.length-1;render()}' +
'function validate(){var errs=[];for(var i=0;i<cards.length;i++){var c=cards[i],code=String(c.code||"");' +
'if(!code.length)errs.push((c.name||"Card "+(i+1))+": code is empty");' +
'else if(code.length>26)errs.push((c.name||"Card")+": code longer than 26 characters");' +
'else{for(var j=0;j<code.length;j++){var cc=code.charCodeAt(j);' +
'if(cc<32||cc>127){errs.push((c.name||"Card")+": unsupported character \\u201c"+code[j]+"\\u201d");break}}}}' +
'return errs}' +
'function save(){var errs=validate();' +
'if(errs.length){alert("Fix before saving:\\n\\n"+errs.join("\\n"));return}' +
'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify(cards))}' +
'render();' +
'<\/script></body></html>';

function buildURL(cards) {
  return 'data:text/html;charset=utf-8,' + encodeURIComponent(PAGE) +
         '#' + encodeURIComponent(JSON.stringify(cards));
}

module.exports = { buildURL: buildURL };
