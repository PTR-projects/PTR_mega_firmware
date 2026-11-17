var getDataIntervalID;var getDataLiveIntervalID;var current_tab=1;const preferencesData={wifi_pass:"your_wifi_pass",main_alt:200,drouge_alt:0,rail_height:2,max_tilt:45,staging_delay:0,staging_max_tilt:45,auto_arming_time_s:60,auto_arming:true,key:0,lora_freq:433125,lora_tracker_freq:434250,lora_mode:true,lora_tx_dbm:0,lora_key:"0",lora_pass:""};const LORA_PASS_MIN=8;const TRACKER_FREQ_KHZ=["434250","434375","434500","434625"];function normalizeLoraPass(value){return String(value==null?"":value).trim().slice(0,63);}
const TAB_IDS=['tab-main','tab-storage','tab-igniters','tab-settings','tab-live'];const MENU_ITEMS=[{el:'menu_home',td:'menu_home_td'},{el:'menu_storage',td:'menu_storage_td'},{el:'menu_ign',td:'menu_ign_td'},{el:'menu_settings',td:'menu_settings_td'},{el:'menu_live',td:'menu_live_td'}];function SelectSection(idx){window.scrollTo(0,0);TabsSelect(idx+1);TAB_IDS.forEach((id,i)=>{document.getElementById(id).style.display=i===idx?'block':'none';});clearInterval(getDataLiveIntervalID);if(idx===3)getPref();if(idx===4)getDataLiveIntervalID=setInterval(getDataLive,1000);}
function SelectSection_Home(){SelectSection(0);}
function SelectSection_Storage(){SelectSection(1);}
function SelectSection_Ign(){SelectSection(2);}
function SelectSection_Settings(){SelectSection(3);}
function SelectSection_Live(){SelectSection(4);}
let touchstartX=0;let touchendX=0;let touchstartY=0;let touchendY=0;function checkDirection(){const verticalDistance=Math.abs(touchstartY-touchendY);const swipeDistance=touchendX-touchstartX;if((Math.abs(swipeDistance)>80)&&(verticalDistance<50)){if(swipeDistance>0)Tab_swipeRight();else Tab_swipeLeft();}}
document.addEventListener('touchstart',e=>{touchstartX=e.changedTouches[0].screenX;touchstartY=e.changedTouches[0].screenY;});document.addEventListener('touchend',e=>{touchendX=e.changedTouches[0].screenX;touchendY=e.changedTouches[0].screenY;checkDirection();});function Tab_swipeRight(){if(current_tab>1)SelectSection(current_tab-2);}
function Tab_swipeLeft(){if(current_tab<5)SelectSection(current_tab);}
function TabsInit(){console.log("Tabs init");SelectSection(0);}
function TabsSelect(num){current_tab=num;MENU_ITEMS.forEach((item,i)=>{const active=i===num-1;const color=active?'black':'#f2f2f2';const bg=active?'#ddd':'#333';const el=document.getElementById(item.el);const td=document.getElementById(item.td);el.style.color=color;el.style.background=bg;td.style.background=bg;});}
function main_arming_handler(){console.log("Main - Arming button pressed");if(confirm('Do you want to Arm?\n\nAfter that command Flight Computer will be ready to flight and igniters will be armed!\n\nUse with caution!'))
POST_simple("/cmd",'{"cmd":"arm","key":2137}');}
function main_disarming_handler(){console.log("Main - Disarming button pressed");if(confirm('Do you want to Disarm?\n\nAfter that command Flight Computer will be disarmed and will not work during flight!\n\nIt will be safe to approach :)'))
POST_simple("/cmd",'{"cmd":"disarm","key":2137}');}
const SFS_PACKET_SIZE=128;const SFS_PREAMBLE=0xAA55;const SFS_PREAMBLE_SCHEMA=0xAA56;const SFS_PAYLOAD_OFFSET=4;const DATAPACKAGE_SIZE=112;const LOG_SCHEMA_MAGIC=0x314C5450;const LOG_SCHEMA_VERSION=1;const LOG_SCHEMA_FLAG_LE=0x01;const LOG_SCHEMA_HDR_SIZE=18;const LOG_SCHEMA_SLICE=104;const LOG_SCHEMA_NAME_MAX=19;const LOG_SCHEMA_MAX_PAYLOAD=122;const LOG_TYPE_I8=1;const LOG_TYPE_U8=2;const LOG_TYPE_I16=3;const LOG_TYPE_U16=4;const LOG_TYPE_I32=5;const LOG_TYPE_U32=6;const LOG_TYPE_F16=7;const LOG_TYPE_F32=8;const LOG_TYPE_F64=9;const CSV_HEADER=['sys_time','accX','accY','accZ','gyroX','gyroY','gyroZ','magX','magY','magZ','accHX','accHY','accHZ','pressure','temp','latitude','longitude','altitude_gnss','gnss_sats','gnss_fix','altitude_press','altitude_kalman','ascent_rate_kalman','tilt','q0','q1','q2','q3','flightstate','ign1_cont','ign2_cont','ign3_cont','ign4_cont','ign1_state','ign2_state','ign3_state','ign4_state','vbat_mV','servo_1','servo_2','servo_3','servo_4','servo_en'].join(';');function pauseStatusPolling(){if(getDataIntervalID){clearInterval(getDataIntervalID);getDataIntervalID=null;}
clearInterval(getDataLiveIntervalID);}
function resumeStatusPolling(){if(!getDataIntervalID){getDataIntervalID=setInterval(getDataStatus,1000);}}
function storage_download_handler(){console.log("Storage - Download pressed");pauseStatusPolling();location.href='/storage/meas.bin';setTimeout(resumeStatusPolling,180000);}
function storage_remove_handler(){console.log("Storage - Remove pressed");if(confirm('Are you sure?'))
POST_simple("/delete/storage/meas.bin",'');}
function crc16_le(bytes,len){let crc=0xFFFF;for(let i=0;i<len;i++){crc^=bytes[i];for(let b=0;b<8;b++){crc=(crc&1)?((crc>>>1)^0xA001):(crc>>>1);}}
return crc&0xFFFF;}
function parseDataPackageCsvLine(view,base){const f=(o)=>view.getFloat32(base+o,true);const u8=(o)=>view.getUint8(base+o);const i8=(o)=>view.getInt8(base+o);const u16=(o)=>view.getUint16(base+o,true);const u32=(o)=>view.getUint32(base+o,true);const gnssRaw=u8(69);const gnssSats=gnssRaw&0x3F;const gnssFix=(gnssRaw>>6)&0x03;const ign=u8(100);return[u32(0),f(4),f(8),f(12),f(16),f(20),f(24),f(28),f(32),f(36),f(40),f(44),f(48),f(52),i8(56),f(57),f(61),f(65),gnssSats,gnssFix,f(70),f(74),f(78),u8(82),f(83),f(87),f(91),f(95),u8(99),(ign>>0)&1,(ign>>1)&1,(ign>>2)&1,(ign>>3)&1,(ign>>4)&1,(ign>>5)&1,(ign>>6)&1,(ign>>7)&1,u16(101),i8(103),i8(104),i8(105),i8(106),u8(107)].join(';');}
function logTypeSize(type){switch(type){case LOG_TYPE_I8:case LOG_TYPE_U8:return 1;case LOG_TYPE_I16:case LOG_TYPE_U16:case LOG_TYPE_F16:return 2;case LOG_TYPE_I32:case LOG_TYPE_U32:case LOG_TYPE_F32:return 4;case LOG_TYPE_F64:return 8;default:return 0;}}
function decodeFloat16(bits){const sign=(bits&0x8000)?-1:1;const exp=(bits>>10)&0x1f;const frac=bits&0x3ff;if(exp===0)
return sign*Math.pow(2,-14)*(frac/1024);if(exp===31)
return frac?NaN:sign*Infinity;return sign*Math.pow(2,exp-15)*(1+frac/1024);}
function readSchemaName(view,offset,len){let name='';for(let i=0;i<len;i++)
name+=String.fromCharCode(view.getUint8(offset+i));return name;}
function readUnsigned(view,offset,size){if(size===1)
return view.getUint8(offset);if(size===2)
return view.getUint16(offset,true);if(size===4)
return view.getUint32(offset,true);const lo=view.getUint32(offset,true);const hi=view.getUint32(offset+4,true);return hi*0x100000000+lo;}
function extractBits(word,bitOff,bitLen){const shifted=bitOff===0?word:Math.floor(word/Math.pow(2,bitOff));const mask=Math.pow(2,bitLen)-1;return Math.floor(shifted)%(mask+1);}
function readMeas(view,offset,field){const size=logTypeSize(field.type);if(field.bitLen!==0)
return extractBits(readUnsigned(view,offset,size),field.bitOff,field.bitLen);switch(field.type){case LOG_TYPE_I8:return view.getInt8(offset);case LOG_TYPE_U8:return view.getUint8(offset);case LOG_TYPE_I16:return view.getInt16(offset,true);case LOG_TYPE_U16:return view.getUint16(offset,true);case LOG_TYPE_I32:return view.getInt32(offset,true);case LOG_TYPE_U32:return view.getUint32(offset,true);case LOG_TYPE_F16:return decodeFloat16(view.getUint16(offset,true));case LOG_TYPE_F32:return view.getFloat32(offset,true);case LOG_TYPE_F64:return view.getFloat64(offset,true);default:return null;}}
function formatField(field,raw){const isFloat=field.type===LOG_TYPE_F16||field.type===LOG_TYPE_F32||field.type===LOG_TYPE_F64;if(!isFloat&&field.scale===1&&field.offset===0)
return String(raw);return String(raw*field.scale-field.offset);}
function newSchemaAssembly(){return{active:false,blob:null,filled:0,next:0,chunkCount:0,blobBytes:0,blobCrc:0,fieldCount:0,sampleBytes:0};}
function readSchemaHeader(view){const base=SFS_PAYLOAD_OFFSET;return{magic:view.getUint32(base,true),version:view.getUint8(base+4),chunkIndex:view.getUint8(base+5),chunkCount:view.getUint8(base+6),flags:view.getUint8(base+7),fieldCount:view.getUint16(base+8,true),sampleBytes:view.getUint16(base+10,true),rateHz:view.getUint16(base+12,true),blobBytes:view.getUint16(base+14,true),blobCrc:view.getUint16(base+16,true)};}
function parseSchemaBlob(asm){const view=new DataView(asm.blob.buffer,asm.blob.byteOffset,asm.blob.byteLength);let pos=0;let cursor=0;const fields=[];if(asm.sampleBytes===0||asm.sampleBytes>LOG_SCHEMA_MAX_PAYLOAD)
return null;for(let i=0;i<asm.fieldCount;i++){if(pos>=asm.blobBytes)
return null;const nameLen=view.getUint8(pos);pos+=1;if(nameLen<1||nameLen>LOG_SCHEMA_NAME_MAX)
return null;if(pos+nameLen+5+8>asm.blobBytes)
return null;const name=readSchemaName(view,pos,nameLen);pos+=nameLen;const type=view.getUint8(pos++);const unit=view.getUint8(pos++);const bitOff=view.getUint8(pos++);const bitLen=view.getUint8(pos++);const advance=view.getUint8(pos++);const scale=view.getFloat32(pos,true);pos+=4;const offset=view.getFloat32(pos,true);pos+=4;const size=logTypeSize(type);if(size===0)
return null;if(bitLen!==0&&bitOff+bitLen>size*8)
return null;if(cursor+size>asm.sampleBytes)
return null;cursor+=advance;if(cursor>asm.sampleBytes)
return null;fields.push({name:name,type:type,unit:unit,bitOff:bitOff,bitLen:bitLen,advance:advance,scale:scale,offset:offset});}
if(pos!==asm.blobBytes)
return null;return fields;}
function acceptSchemaFrame(view,asm){const hdr=readSchemaHeader(view);if(hdr.magic!==LOG_SCHEMA_MAGIC)
return asm.fields||null;if(hdr.version!==LOG_SCHEMA_VERSION||hdr.flags!==LOG_SCHEMA_FLAG_LE){asm.active=false;return asm.fields||null;}
if(hdr.chunkCount===0||hdr.chunkIndex>=hdr.chunkCount){asm.active=false;return asm.fields||null;}
if(hdr.chunkIndex===0){asm.active=true;asm.blob=new Uint8Array(hdr.blobBytes);asm.filled=0;asm.next=0;asm.chunkCount=hdr.chunkCount;asm.blobBytes=hdr.blobBytes;asm.blobCrc=hdr.blobCrc;asm.fieldCount=hdr.fieldCount;asm.sampleBytes=hdr.sampleBytes;}
if(!asm.active||hdr.chunkIndex!==asm.next||hdr.chunkCount!==asm.chunkCount||hdr.blobBytes!==asm.blobBytes||hdr.blobCrc!==asm.blobCrc||hdr.fieldCount!==asm.fieldCount||hdr.sampleBytes!==asm.sampleBytes){asm.active=false;return asm.fields||null;}
const blobOffset=hdr.chunkIndex*LOG_SCHEMA_SLICE;if(blobOffset>=hdr.blobBytes||hdr.blobBytes>asm.blob.length){asm.active=false;return asm.fields||null;}
let slice=hdr.blobBytes-blobOffset;if(slice>LOG_SCHEMA_SLICE)
slice=LOG_SCHEMA_SLICE;if(SFS_PAYLOAD_OFFSET+LOG_SCHEMA_HDR_SIZE+slice>view.byteLength){asm.active=false;return asm.fields||null;}
if(asm.filled+slice>hdr.blobBytes){asm.active=false;return asm.fields||null;}
const src=new Uint8Array(view.buffer,view.byteOffset+SFS_PAYLOAD_OFFSET+LOG_SCHEMA_HDR_SIZE,slice);asm.blob.set(src,asm.filled);asm.filled+=slice;asm.next++;if(asm.next!==asm.chunkCount)
return asm.fields||null;if(asm.filled!==asm.blobBytes||crc16_le(asm.blob,asm.blob.length)!==asm.blobCrc){asm.active=false;return asm.fields||null;}
const fields=parseSchemaBlob(asm);asm.active=false;if(!fields)
return asm.fields||null;asm.fields=fields;return fields;}
function decodeSchemaLine(view,base,fields){let cursor=0;const parts=[];for(let i=0;i<fields.length;i++){const field=fields[i];const size=logTypeSize(field.type);if(base+cursor+size>view.byteLength)
return null;const raw=readMeas(view,base+cursor,field);if(raw===null)
return null;parts.push(formatField(field,raw));cursor+=field.advance;}
return parts.join(';');}
function triggerCsvDownload(blobParts){const blob=new Blob(blobParts,{type:'text/csv'});const url=URL.createObjectURL(blob);const a=document.createElement('a');a.href=url;a.download='meas.csv';document.body.appendChild(a);a.click();document.body.removeChild(a);URL.revokeObjectURL(url);}
async function storage_download_csv_handler(){console.log("Storage - Download CSV pressed");const btn=document.getElementById('button-log-download-csv');const prevLabel=btn?btn.textContent:'';if(btn){btn.disabled=true;btn.textContent='Converting...';}
pauseStatusPolling();try{const response=await fetch('/storage/meas.bin');if(!response.ok)
throw new Error('HTTP '+response.status);const reader=response.body.getReader();const csvParts=[];const schemaAsm=newSchemaAssembly();let schemaFields=null;let headerWritten=false;let leftover=new Uint8Array(0);let rowCount=0;let skipped=0;while(true){const{done,value}=await reader.read();if(done)break;const merged=new Uint8Array(leftover.length+value.length);merged.set(leftover,0);merged.set(value,leftover.length);let offset=0;while(offset+SFS_PACKET_SIZE<=merged.length){const packet=merged.subarray(offset,offset+SFS_PACKET_SIZE);const view=new DataView(packet.buffer,packet.byteOffset,packet.byteLength);const pre=view.getUint16(0,true);const crcStored=view.getUint16(126,true);const crcCalc=crc16_le(packet,126);if(crcStored===crcCalc&&pre===SFS_PREAMBLE_SCHEMA){const installed=acceptSchemaFrame(view,schemaAsm);if(installed)
schemaFields=installed;}else if(crcStored===crcCalc&&pre===SFS_PREAMBLE){let line;if(schemaFields)
line=decodeSchemaLine(view,SFS_PAYLOAD_OFFSET,schemaFields);else
line=parseDataPackageCsvLine(view,SFS_PAYLOAD_OFFSET);if(line===null){skipped++;}else{if(!headerWritten){const header=schemaFields?schemaFields.map(function(field){return field.name;}).join(';'):CSV_HEADER;csvParts.push(header+'\n');headerWritten=true;}
csvParts.push(line+'\n');rowCount++;}}else{skipped++;}
offset+=SFS_PACKET_SIZE;}
leftover=merged.subarray(offset);}
if(rowCount===0){alert(skipped>0?'No valid measurement packets found (skipped '+skipped+').':'Measurement log is empty.');return;}
triggerCsvDownload(csvParts);console.log('CSV download ready: '+rowCount+' rows, skipped '+skipped);}catch(err){console.error('CSV download failed',err);alert('CSV download failed: '+(err&&err.message?err.message:err));}finally{resumeStatusPolling();if(btn){btn.disabled=false;btn.textContent=prevLabel;}}}
function vibrate(time){if('vibrate'in navigator)navigator.vibrate(time);}
function ign_unlock_handler(n){const fireBtn=document.getElementById('button-ign'+n+'-fire');const unlockBtn=document.getElementById('button-ign'+n+'-unlock');const lockLabel=document.getElementById('label-igniter'+n+'-lock-status');if(!fireBtn.disabled){unlockBtn.textContent='UNLOCK';fireBtn.disabled=true;lockLabel.textContent='Status Locked';lockLabel.style.color='black';return;}
vibrate(200);setTimeout(function(){if(confirm('Are you sure? Igniter '+n+' will be unlocked!')){unlockBtn.textContent='SECURE';fireBtn.disabled=false;lockLabel.textContent='Unlocked!';lockLabel.style.color='red';}},200);}
function ign_ign1_unlock_handler(){ign_unlock_handler(1);}
function ign_ign2_unlock_handler(){ign_unlock_handler(2);}
function ign_ign3_unlock_handler(){ign_unlock_handler(3);}
function ign_ign4_unlock_handler(){ign_unlock_handler(4);}
function ign_fire_handler(n){console.log("Igniters - Fire ign "+n);vibrate(200);POST_simple("/cmd",'{"cmd":"ign_set","arg1":'+n+',"key":2137}');}
function ign_ign1_fire_handler(){ign_fire_handler(1);}
function ign_ign2_fire_handler(){ign_fire_handler(2);}
function ign_ign3_fire_handler(){ign_fire_handler(3);}
function ign_ign4_fire_handler(){ign_fire_handler(4);}
function POST_simple(url,data){return fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:data});}
function setLabel(id,text,color){const el=document.getElementById(id);el.textContent=text;el.style.color=color;el.style.fontWeight='bold';}
function SysMgr_statusToLabel(status,label_id){if(status===0x01)return setLabel(label_id,'OK','green');if(status===0x02)return setLabel(label_id,'WARNING','red');if(status===0x04)return setLabel(label_id,'FAIL','red');setLabel(label_id,'ERROR','red');}
function SysMgr_armingStatusToLabel(status,label_id){if(status===0x01)return setLabel(label_id,'ARMED!','green');if(status===0x02)return setLabel(label_id,'Disarmed!','red');if(status===0x04)return setLabel(label_id,'ERROR :(','red');setLabel(label_id,'ERROR','red');}
function GpsFix_fixToLabel(status,label_id){if(status===0)return setLabel(label_id,'No Fix','red');if(status===1)return setLabel(label_id,'Fix OK','green');setLabel(label_id,'ERROR','red');}
function getDataStatus(){fetch("/status").then(r=>r.ok?r.json():Promise.reject('Error '+r.status)).then(data=>{console.log("Refresh Status OK");document.getElementById('label-status-serial-number').textContent=data.configuration.serial_number;document.getElementById('label-status-software-version').textContent=data.configuration.software_version;SysMgr_armingStatusToLabel(data.sysMgr.sysmgr_arm_state,'label-status-arming');document.getElementById('label-status-timestamp_ms').textContent=data.system.timestamp_ms+' ms';SysMgr_statusToLabel(data.sysMgr.sysmgr_system_status,'label-status-system');SysMgr_statusToLabel(data.sysMgr.sysmgr_analog_status,'label-status-analog');SysMgr_statusToLabel(data.sysMgr.sysmgr_lora_status,'label-status-lora');SysMgr_statusToLabel(data.sysMgr.sysmgr_adcs_status,'label-status-adcs');SysMgr_statusToLabel(data.sysMgr.sysmgr_storage_status,'label-status-storage');SysMgr_statusToLabel(data.sysMgr.sysmgr_sysmgr_status,'label-status-sysmgr');SysMgr_statusToLabel(data.sysMgr.sysmgr_utils_status,'label-status-utils');SysMgr_statusToLabel(data.sysMgr.sysmgr_web_status,'label-status-web');document.getElementById('label-status-pressure').textContent=data.sensors.pressure;document.getElementById('label-status-angle').textContent=data.sensors.rocket_tilt.toFixed(2)+' deg';GpsFix_fixToLabel(data.sensors.gpsfix,'label-status-gpsstat');document.getElementById('label-status-gpssats').textContent=data.sensors.gpssats;if(data.system.battery_voltage>3.0)
document.getElementById('label-status-vbat').textContent=data.system.battery_voltage.toFixed(2)+' V';else
document.getElementById('label-status-vbat').textContent='LOW!';IGN_contToLabel(data.igniters[0].continuity,'label-igniter1-cont-status');IGN_contToLabel(data.igniters[1].continuity,'label-igniter2-cont-status');IGN_contToLabel(data.igniters[2].continuity,'label-igniter3-cont-status');IGN_contToLabel(data.igniters[3].continuity,'label-igniter4-cont-status');}).catch(console.error);}
function IGN_contToLabel(cont,label){const text=cont===0?'Missing!':cont===1?'Connected':'BAT missing!';const color=cont===0?'red':cont===1?'green':'orange';setLabel(label,text,color);}
function webInit(){TabsInit();getDataIntervalID=setInterval(getDataStatus,1000);initDataLive();getDataStatus();}
function makeElementId(category,key){return category.replace(/\s/g,'_')+'-'+key.replace(/\s/g,'_')+'-value';}
function setLiveValue(el,value){if(typeof value==='number')
el.textContent=Number.parseFloat(value).toPrecision(6);else if(typeof value==='string')
el.textContent=value;else
el.textContent='NaN';}
function createLiveTable(json){if(typeof json==='undefined'){console.log("JSON does not exist!");return;}
const entries=Object.entries(json);if(entries.length===0){console.log("JSON length equal to zero!");return;}
const body=document.getElementById('tab-live');const tbl=document.createElement('table');tbl.className='table-live';for(let i=0;i<entries.length;i++){if(i>0){const spacer=tbl.insertRow();spacer.insertCell();spacer.insertCell();spacer.style.backgroundColor='#f2f2f2';}
const headerRow=tbl.insertRow();const headerCell=headerRow.insertCell();headerCell.textContent=entries[i][0];headerCell.style.fontWeight='bold';const object_entries=Object.entries(entries[i][1]);for(let j=0;j<object_entries.length;j++){const row=tbl.insertRow();row.insertCell().textContent=object_entries[j][0];const lbl=document.createElement('label');lbl.id=makeElementId(entries[i][0],object_entries[j][0]);setLiveValue(lbl,object_entries[j][1]);row.insertCell().appendChild(lbl);}}
body.appendChild(tbl);}
function updateLiveTable(json){if(typeof json==='undefined'){console.log("JSON does not exist!");return;}
const entries=Object.entries(json);if(entries.length===0){console.log("JSON length equal to zero!");return;}
for(let i=0;i<entries.length;i++){const object_entries=Object.entries(entries[i][1]);for(let j=0;j<object_entries.length;j++){const el=document.getElementById(makeElementId(entries[i][0],object_entries[j][0]));if(el)setLiveValue(el,object_entries[j][1]);}}}
function fetchLive(callback){fetch('/live').then(r=>r.ok?r.json():Promise.reject('Error '+r.status)).then(data=>{console.log("Refresh Status OK");callback(data);}).catch(console.error);}
function initDataLive(){fetchLive(createLiveTable);}
function getDataLive(){fetchLive(updateLiveTable);}
function calculateCRC16(data){const poly=0x1021;let crc=0xffff;for(let i=0;i<data.length;i++){crc^=data.charCodeAt(i)<<8;for(let j=0;j<8;j++)
crc=(crc&0x8000)?((crc<<1)^poly):(crc<<1);}
return crc&0xffff;}
function sendPreferencesData(preferencesData){updatePreferencesData();if(preferencesData.lora_pass.length<LORA_PASS_MIN){alert("LoRa password must be at least 8 characters.");return;}
if(confirm('Save and restart? If you changed WiFi password then change your phone settings too.')){const jsonData=JSON.stringify(preferencesData,null,'\t');const crc16=calculateCRC16(jsonData);console.log("Sending preferencesData:",preferencesData);POST_simple('/config',crc16+','+jsonData);setTimeout(getPref,5000);}}
function formatWifiPass(wifiPass){if(wifiPass.length>12){return wifiPass.substring(0,12);}else{return wifiPass;}}
function updatePreferencesData(){preferencesData.rail_height=parseInt(1000*document.getElementById('pref-launchpad-height').value);preferencesData.main_alt=parseInt(document.getElementById('pref-main-alt').value);preferencesData.drouge_alt=parseInt(document.getElementById('pref-drouge-alt').value);preferencesData.staging_delay=parseInt(1000*document.getElementById('pref-staging-delay').value);preferencesData.staging_max_tilt=parseInt(document.getElementById('pref-staging-tilt').value);preferencesData.auto_arming_time_s=parseInt(document.getElementById('pref-autoarm-delay').value);preferencesData.auto_arming=parseInt(document.getElementById('pref-autoarm-enable').selectedIndex);preferencesData.lora_freq=parseInt(1000*document.getElementById('pref-lora-frequency').value);preferencesData.lora_tracker_freq=parseInt(document.getElementById('pref-lora-tracker-frequency').value,10);preferencesData.lora_mode=parseInt(document.getElementById('pref-lora-mode').selectedIndex);preferencesData.lora_pass=normalizeLoraPass(document.getElementById('pref-lora-pass').value);preferencesData.lora_key="0";preferencesData.key=0;preferencesData.lora_tx_dbm=0;preferencesData.wifi_pass=document.getElementById('pref-wifi-pass').value;console.log("Updated preferencesData:",preferencesData);}
function preferencesDataLoraMode(state){preferencesData.lora_mode=state;}
function preferencesDataAutoarming(state){preferencesData.auto_arming=state;}
function getPref(){fetch('/pref').then(r=>r.ok?r.json():Promise.reject('Error '+r.status)).then(data=>{console.log("Refresh Pref OK");console.log(data);document.getElementById('pref-launchpad-height').value=parseFloat(data.pref_launchpad_height*0.001);document.getElementById('pref-main-alt').value=parseInt(data.pref_main_alt);document.getElementById('pref-drouge-alt').value=parseInt(data.pref_drouge_alt);document.getElementById('pref-staging-delay').value=parseFloat(data.pref_staging_delay*0.001);document.getElementById('pref-staging-tilt').value=parseInt(data.pref_staging_tilt);document.getElementById('pref-autoarm-delay').value=parseInt(data.pref_autoarm_delay);document.getElementById('pref-lora-frequency').value=parseFloat(data.pref_lora_frequency*0.001);const trackerFreq=document.getElementById('pref-lora-tracker-frequency');const trackerKhz=String(parseInt(data.pref_lora_tracker_frequency,10));trackerFreq.value=TRACKER_FREQ_KHZ.indexOf(trackerKhz)>=0?trackerKhz:TRACKER_FREQ_KHZ[0];document.getElementById('pref-wifi-pass').value=data.pref_wifi_pass;document.getElementById('pref-autoarm-enable').selectedIndex=parseInt(data.pref_autoarming);document.getElementById('pref-lora-mode').selectedIndex=parseInt(data.pref_lora_mode);document.getElementById('pref-lora-pass').value=typeof data.pref_lora_pass==="string"?data.pref_lora_pass:"";updatePreferencesData();preferencesData.wifi_pass=document.getElementById('pref-wifi-pass').value;}).catch(console.error);}