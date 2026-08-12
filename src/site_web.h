// HTML web page 
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="apple-mobile-web-app-capable" content="no">
<meta name="mobile-web-app-capable" content="no">
<title>Temp-Humid</title>
<style>
  body {font-family: Arial,Helvetica,sans-serif;background: #181818;color: #EFEFEF;font-size: 16px}
  section.main { display: flex }
  #menu,section.main {flex-direction: column}
  #menu {display: none; flex-wrap: nowrap;min-width: 340px;background: #363636;padding: 8px;border-radius: 4px;margin-top: -10px;margin-right: 10px;}
  #content {display: flex;flex-wrap: wrap;align-items: stretch}
  section#buttons { display: flex;flex-wrap: nowrap;justify-content: space-between }
  #nav-toggle {cursor: pointer;display: block}
  #nav-toggle-cb {outline: 0;opacity: 0;width: 0;height: 0}
  #nav-toggle-cb:checked+#menu {display: flex}
  .alert-icon {  color: #d00000; margin-right: 6px; }
  .forcage-label { color: #d00000;   /* rouge lisible */  font-weight: 600; /* optionnel */}
  .input-group { display: flex; flex-wrap: nowrap; line-height: 22px; margin: 5px 0 }

  .input-group>label { display: inline-block; padding-right: 10px;  min-width: 10px }
  .input-group input,.input-group select {  flex-grow: 1 }
  .input2-group {  display: flex; flex-direction: column; gap: 6px; }
  .input2-group>label { display: inline-block; padding-right: 10px;  min-width: 10px }
  .input2-group input,.input2-group select {  flex-grow: 1 }
  .switch { display: block;position: relative;line-height: 22px;font-size: 16px;height: 22px}
  .switch input {outline: 0;opacity: 0;width: 0;height: 0}
  .slider {width: 50px;height: 22px;border-radius: 22px;cursor: pointer;background-color: grey}
  .slider,.slider:before {display:inline-block; transition: 1.2s}
  .slider:before {
                position: relative;
                content: "";
                border-radius: 10px;
                height: 16px;
                width: 16px;
                left: 4px;
                top: 3px;
                background-color: #fff
            }
  input:checked+.slider {background-color: grey}
  input:checked+.slider:before { -webkit-transform: translateX(26px); transform: translateX(26px) }
  label.toggle-section-label { cursor: pointer; display: block }
  input.toggle-section-button {outline: 0;opacity: 0;width: 0;height: 0}
  input.toggle-section-button:checked+section.toggle-section {display: none}
  canvas.graph-group {
      width: 300px;  /* Fixe la largeur d'affichage */
      height: 195px; /* Fixe la hauteur d'affichage */
      display: block; /* Évite les comportements inline bizarres */
  }
  .cancel-btn {    margin-left: 8px;
    width: 26px;
    height: 26px;
    border-radius: 50%;
    border: none;
    background-color: #e74c3c;
    color: white;
    font-size: 16px;
    font-weight: bold;
    cursor: pointer;
    line-height: 26px;
    text-align: center;}
  .cancel-btn:hover {background-color: #c0392b;}
  .led-red {
    background:#fe0206;
    border-radius:10px;
    width:18px;
    height:18px;
    border:2px solid #f8f9f8;
  }  
  .led-no {
    background:#363636;
    border-radius:10px;
    width:18px;
    height:18px;
  }  
  .temperature {display: flex; gap: 15px; font-size: 18px;}
   h1 {font-size: 22px;  margin: 10px 0; }
  table.pgm { width: 100%; border-collapse: collapse; margin-top: 10px; font-size: 14px; background: #222; }
  table.pgm th, table.pgm td { border: 1px solid #555; padding: 4px; text-align: center; }
  table.pgm input { width: 40px; background: #333; color: #fff; border: 1px solid #666; text-align: center; }
  table.pgm select { background: #333; color: #fff; border: 1px solid #666; font-size: 12px; }
.inline-group { display: flex;  align-items: center; gap: 12px; }
#batS-group { display: none; margin-left: 20px; }
#co_fi-group { display: none; margin-left: 20px; }
#vac-d-group { display: none; flex-direction: column; width: 100%; margin-top: 6px; }
#vac-d-group .input-group { padding-left: 20px; }
.vac-details { width: 100%; display: flex; flex-direction: column; margin-left: 6px;  margin-top: 6px; }
#Tint-mess-group { display: none; margin-left: 20px; }
#jusque-group { display: none; margin-left: 20px; }
#consigne-group { position: relative; display: inline-block; }

.dropdown {
    position: absolute;
    left: 50%;
    transform: translateX(-50%);
    background: #fff;
    border: 1px solid #ccc;
    max-height: 150px;
    overflow-y: auto;
    box-shadow: 0 2px 5px rgba(0,0,0,0.2);
    z-index: 100;
}

.dropdown div {
    padding: 5px 10px;
    cursor: pointer;
    text-align: center;
}

.dropdown div:hover {
    background-color: #eee;
}
</style>
</head>
<body>
    <h2>Temp-Humid</h2>
    <h3></h3>
    <section class="main">
      <div id="logo">
        <label for="nav-toggle-cb" id="nav-toggle">&#9776;&nbsp;&nbsp;Pilotage</label>
      </div>
      <div id="content">
        <div id="sidebar">
          <input type="checkbox" id="nav-toggle-cb" checked="checked">
          <nav id="menu">

            <button onclick="fetchStatusData()">maj</button>

            <div class="input-group">
                <label for="Text">Temp Ext:</label>
                <div class="text">
                    <span id="Text" class="default-action"></span>&deg;C&nbsp&nbsp&nbsp
                </div>
            </div>

            <div class="input-group">
                <label for="Tint">Temp Int:</label>
                <div class="text">
                    <span id="Tint" class="default-action"></span>&deg;C&nbsp&nbsp&nbsp
                </div>
            </div>

            <div class="input-group">
                <label for="Humid">Humidité :</label>
                <div class="text">
                    <span id="Humid" class="default-action"></span>
                </div>
            </div>

            <div class="input-group">
                <label for="batS">Tension batterie :</label>
                <div class="text">
                    <span id="batS" class="default-action"></span>&nbspV
                </div>
            </div>

            <div class="input-group" id="batS-group">
              <label for="batS"></label>
              <div class="text">
                <span class="alert-icon">⚠️</span>
                Batterie faible :
                <span id="batS" class="default-action"></span> V
              </div>
            </div>

            <div class="input-group">
                <label for="TextV">Temp Ext veille:</label>
                <div class="text">
                    <span id="TextV" class="default-action"></span>&deg;C&nbsp&nbsp&nbsp
                </div>
            </div>

            <div class="input-group">
                <label for="TintV">Temp Int veille :</label>
                <div class="text">
                    <span id="TintV" class="default-action"></span>&deg;C&nbsp&nbsp&nbsp
                </div>
            </div>

            <div class="input-group">
                <label for="Hum_V">Humidité veille :</label>
                <div class="text">
                    <span id="Hum_V" class="default-action"></span>
                </div>
            </div>


            <p>Graphique Temp-humidite : </p>
            <canvas id = "schema" height="195" width="300" style="border:1px solid" class="graph-group">
              Votre navigateur ne supporte pas la balise canvas
            </canvas>

            <div class="input-group" id="pac-group">
                <label for="coche_secu">code      </label>
                <p STYLE="padding:0 0 0 6px;"></p>
                <div class="switch">
                    <input id="coche_secu" type="checkbox" class="default-action">
                    <label class="slider" for="coche_secu"></label>
                </div>
                <div class="default-action">
                  <div class="container">
                    <div id="codeR_secu" class="default-action"></div>
                  </div>
                </div>
                <p STYLE="padding:0 0 0 25px;"></p>
              <input id="code_secu" type="text"  size="7"  class="default-action">
            </div>

            <div>
              <input id="nb_capteurs" type="hidden" name="nb_capteurs" value="%NB_CAPTEURS%" class="lecture_var">
            </div>

            <p>Uptime: <span id="uptime" class="default-action"></p>

          </nav>

      

          <hr style="width:150px">
          <label for="nav-toggle-det" class="toggle-section-label">&#9776;&nbsp;&nbsp;Détail valeurs</label><input type="checkbox" id="nav-toggle-det" class="hidden toggle-section-button" checked="checked">
          <section class="toggle-section">

          <button onclick="fetchStatusData()">maj</button>


            <div class="input-group">
              <label for="PPE">Prochaine maj cycle :</label>
              <div class="text">
                  <span id="PPE" class="default-action"></span> sec
              </div>
            </div>



            <div class="input-group">
                <label for="periode_c">Periode cycle :</label>
                <div class="text">
                    <span id="periode_cycle" class="default-action"></span> min
                </div>
            </div>

            <p>Graphique 24h : </p>
            <canvas id = "schema2" height="195" width="300" style="border:1px solid" class="graph-group">
              Votre navigateur ne supporte pas la balise canvas
            </canvas>

          </section>          

          <hr style="width:150px">
          <label for="nav-toggle-reg" class="toggle-section-label">&#9776;&nbsp;&nbsp;Register Get/ Set</label>
          <input type="checkbox" id="nav-toggle-reg" class="hidden toggle-section-button" checked="checked">
          <section class="toggle-section">
              <div class="input-group" id="set-reg-group">
                  <label for="set-reg" style="width:120px">Reg, Value :</label>
                  <div class="text">
                      <input id="reg-addr" type="text" minlength="4" maxlength="6" size="6" value="3">
                  </div>
                  <div class="text">
                      <input id="reg-value" type="text" minlength="4" maxlength="6" size="6" value="128">
                  </div>
                  <button class="inline-button" id="set-reg">Set</button>
              </div>
              <div class="input-group" id="get-reg-group">
                  <label for="get-reg" style="width:120px">Reg :</label>
                  <div class="text">
                      <input id="get-reg-addr" type="text" minlength="4" maxlength="6" size="6" value="3">
                  </div>
                  <button class="inline-button" id="get-reg">Get</button>
              </div>
              <div class="input-group">
                  <label for="get-reg-value" style="width:120px">Value:</label>
                  <div class="text">
                      <span id="get-reg-value">12.1</span>
                  </div>
              </div>
              <hr style="width:100px">
              <p> 1 : mode reseau (11-12:AP, 13:routeur, 14:filaire)<br>
                  2 : Nb_reset<br>
                  3 : reset-13<br>
                  4 : Periode cycle<br>
                  5 : cycle rapide - 0(h)-12(min)<br>
                  6 : log detail : 0:rien 4:max<br>
                  7 : délai écoute websocket<br>
                  8 : Skip graph : 1 valeur sur X<br>
                  9 : Seuil batt sonde<br>
                  10: Freq Log batterie(jours)<br>
                  11: réglage date<br>
                  12: réglage heure<br>
                  13 : activation OTA<br>
                  14 : pause entre detections (sec)<br>
                  15 : seuil bas arret ESP<br>
                  16 : duree allumage (sec)<br>
                  17 : action : stockage<br>
                  18 : action : envoi<br>
                  19 : frequence envoi<br>
                  40 : activ esp_now<br>
                  41 : canal wifi <br>
                  42 : canal wifi prérentiel (sonde)<br>
                  43 : puissance wifi<br>
                  44 : wifi_sleep (0,1,2)<br>
                  45 : (L)rssi wifi<br>
                  46 : calib hygro30<br>
                  47 : calib hygro80<br>
                  48 : calib température<br>
                  <br>

              <div class="input-group" id="set-regT-group">
                  <label for="set-regT" style="width:120px">Reg, Value Texte:</label>
                  <div class="text">
                      <input id="regT-addr" type="text" minlength="4" maxlength="6" size="4" value="1">
                  </div>
                  <div class="text">
                      <input id="regT-value" type="text" minlength="4" maxlength="36" size="16" placeholder="192.168.1.1">
                  </div>
                  <button class="inline-button" id="set-regT">Set texte</button>
              </div>
              <div class="input-group" id="get-regT-group">
                  <label for="get-regT" style="width:120px">Reg :</label>
                  <div class="text">
                      <input id="get-regT-addr" type="text" minlength="4" maxlength="6" size="6" value="1">
                  </div>
                  <button class="inline-button" id="get-regT">Get texte</button>
              </div>
              <div class="input-group">
                  <label for="get-regT-value" style="width:120px">Value:</label>
                  <div class="text">
                      <span id="get-regT-value">1</span>
                  </div>
              </div>
              <hr style="width:100px">
              <p> 1 : adresse IP<br>
                  2 : gateway IP<br>
                  3 : subnet IP<br>
                  4 : DNS primaire<br>
                  5 : DNS secondaire<br>
                  6 : nom routeur<br>
                  7 : mdp routeur<br>
                  8 : websocket On (1-2)<br>
                  9 : websocket<br>
                  10 : websock id<br>
                  11 : (L)adresse Mac module<br>
                  12 : adresse Mac Serveur<br>
    

            <div>
            <h1>Log erreurs: 1(Tint) 2(Text) 3(heure)..</h1>
            <p id='erreurs_histo'><br></p>
            <h1>Log :</h1>
            <p id='log_histo'><br></p>
            </div>

            <div>
              <label for="countInput">Nombre de logs à afficher :</label>
              <input type="number" id="countInput" min="1" max="100" value="6" />
              <button id="loadLogsBtn">Charger les logs</button>
            </div>
            <br>
            <div>
              <label for="Action_reg">Action :</label>
              <input id="Action_reg" type="text" minlength="4" maxlength="8" size="8" placeholder="action">
              <input id="Action_data" type="text" minlength="4" maxlength="8" size="8" placeholder="valeur">
              <button id="ActionBtn">Lancer Action</button>
            </div>
            <pre id="actionResult"></pre>

          </section>          
        </div>
      </div>

      <div>Version : <span  id="version" class="default-action"></span></div>
      <span id="heure_string" class="default-action"></span>
      <div>Reset 0:<span  id="reset0" class="default-action"></span>  </div>
      <br>
      <div>Nb_reset:<span  id="nb_reset" class="default-action"></div>
      <br>
      <div>Duree_prec_fct : <span  id="duree_reset" class="default-action"></span></div>
    </section>





    <script>
    NB_Val_Graph=99;
    Nombre_capteurs=0;
    const NB_GRAPH = 6;
    let graphique = [];
    for (let i = 0; i < NB_GRAPH; i++) { graphique[i] = new Array(NB_Val_Graph); }
    
    NB_erreurs=10;
    erreur_code = [];
    erreur_valeur= [];
    erreur_val2= [];
    erreur_jour = [];
    erreur_heure = [];
    erreur_minute = [];
    maj=0;
    fo_co=0;
    

    function stepsToTime(v) {
        let totalMins = parseInt(v) * 10;
        let h = Math.floor(totalMins / 60);
        let m = totalMins % 60;
        return h + "h" + (m < 10 ? "0" + m : m);
    }
    
    function timeToSteps(t) {
        let parts = t.toString().split(/[h:H]/);
        if (parts.length < 2) {
            let val = parseInt(t);
            return isNaN(val) ? 0 : val;
        }
        let h = parseInt(parts[0]) || 0;
        let m = parseInt(parts[1]) || 0;
        return Math.min(255, Math.round((h * 60 + m) / 10));
    }

    document.addEventListener('DOMContentLoaded', function (event) {
       var baseHost = document.location.origin;

      function VisibiliteGroup(groupId, visible) {
          const el = document.getElementById(groupId);
          if (!el) return;
          el.style.display = (visible && visible != 0) ? "flex" : "none";
      }

      const scale = {
          co_fi: 10,
          fo_co: 10,
          consigne: 10,
          va_cons: 10,
          cy_cha: 10,
          LRTT:60,
          TextV:10,
          CoutV:5,
          TintV:10,
          TextV:10,
          Hum_V:10
      };


      
      const updateValue = (el, value, updateRemote) => {   // mise à jour de la valeur sur la page web
        updateRemote = updateRemote == null ? true : updateRemote
        let initialValue
        
        let ledEl = document.getElementById(el.id + "_led");
        if (ledEl) {
          if (value) {
            ledEl.classList.remove('led-no');
            ledEl.classList.add('led-red');
          } else {
            ledEl.classList.remove('led-red');
            ledEl.classList.add('led-no');
          }
        }

        if (el.classList.contains('visibi'))  {
          if (value != 0)
          {
            el.style.display = 'inline';
          }
          else
          {
            el.style.display = 'none';
          }
        }

        if (el.parentElement.className == 'container')  {
          if (value)
          {
            el.classList.remove('led-no');
            el.classList.add('led-red');
          }
          else
          {
            el.classList.remove('led-red');
            el.classList.add('led-no');
          }
        }
        else if (el.nodeName == 'SPAN' && !el.classList.contains('visibi'))  {
          el.innerHTML = value
        }
        else
        {
          if (el.type === 'checkbox') {
            initialValue = el.checked
            value = !!value
            el.checked = value
          } else {
            initialValue = el.value
            el.value = value
            const valueEl = document.getElementById(el.id + "_value");
            if (valueEl) {
              valueEl.textContent = value;            
            }
          }
        }
        if (el.id === "fo_jus") {
            VisibiliteGroup("jusque-group", parseInt(value));
        }
        if (el.id === "cons_fixe") {
             VisibiliteGroup("co_fi-group", value ? 1 : 0);
        }
        if (el.id === "vacances") {
             VisibiliteGroup("vac-d-group", value ? 1 : 0);
        }

        if (updateRemote && initialValue !== value) {
          updateConfig(el);
        }
        else if(!updateRemote)
        {
          if(el.id === "awb_gain"){
            value ? show(wb) : hide(wb)
          }
        }
      }

      function get_value (id)  {    // requete get pour récuperer une valeur, et mise à jour sur la page web

        let el = document.getElementById(id);

        const query = `${baseHost}/Get?type=1&reg=${el.id}`

        fetch(query)
          .then(function (response) {
              return response.json()
          })
          .then(function (state) {
              if (state.val !== undefined) {
                let value = state.val;
                if (scale[el.id]) {
                  value = (value / scale[el.id]).toFixed(1);
                }
                updateValue(el, value, false);
              }
          })
      }

      const setRegButton = document.getElementById('set-reg')
      setRegButton.onclick = () => {
        let reg = parseInt(document.getElementById('reg-addr').value);
        let value = parseFloat(document.getElementById('reg-value').value);
        let getValueField = document.getElementById('get-reg-value');

        const query = `${baseHost}/Set?type=2&reg=${reg}&val=${value}`

        fetch(query)
         .then(response => {
           return response.json();
        })
        .then(state => {
            if (state.reg !== undefined && state.val !== undefined) {
                updateValue(getValueField, state.val, false);
            }
        })
      };

      const getRegButton = document.getElementById('get-reg')
      getRegButton.onclick = () => {
        let reg = parseInt(document.getElementById('get-reg-addr').value);
        let value = document.getElementById('get-reg-value');

        if (isNaN(reg)) {
            console.error("Adresse du registre invalide !");
            value.innerHTML = "Erreur : adresse invalide";
            return;
        }
        const query = `${baseHost}/Get?type=2&reg=${reg}`

        fetch(query)
          .then(response => {
            if (!response.ok) {
                throw new Error(`Erreur HTTP ${response.status}`);
            }
            return response.json()
          })
          .then( state => {
              if (state.reg !== undefined && state.val !== undefined) {
                  value.innerHTML = state.val;
              } else {
                  value.innerHTML = "Erreur : réponse invalide";
              }

          })
          .catch(error => {
            console.error("Erreur lors de la requête :", error);
            value.innerHTML = "Erreur de requête";
          });
      };

      const setRegTButton = document.getElementById('set-regT')
      setRegTButton.onclick = () => {
        let reg = parseInt(document.getElementById('regT-addr').value);
        let value = document.getElementById('regT-value').value.trim(); 
        let getValueField = document.getElementById('get-regT-value');

        const query = `${baseHost}/Set?type=4&reg=${reg}&val=${encodeURIComponent(value)}`;

        fetch(query)
          .then(response => {
              if (!response.ok) {
                  throw new Error(`Erreur HTTP ${response.status}`);
              }
              return response.json();
          })
          .then(state => {
              if (state.reg !== undefined && state.val !== undefined) {
                  updateValue(getValueField, state.val, false);
              }
          })
          .catch(error => {
              console.error("Erreur lors de la requête :", error);
          });
        };

      const getRegTButton = document.getElementById('get-regT')
      getRegTButton.onclick = () => {
        let reg = parseInt(document.getElementById('get-regT-addr').value);
        let value = document.getElementById('get-regT-value');

        const query = `${baseHost}/Get?type=4&reg=${reg}`

        fetch(query)
          .then(function (response) {
              return response.json()
          })
          .then(function (state) {
            if (state.reg !== undefined && state.val !== undefined) {
                  value.innerHTML = state.val;
            }
          })
      }

      function sendValueToESP(id, value) {
          const query = `${baseHost}/Set?type=1&reg=${id}&val=${value}`;

          fetch(query)
            .then(response => {
                setTimeout(() => { get_value(id); }, 1000);
            });
      }


      function updateConfig (el) {   // modif sur page web : requete / set pour mettre a jour l'esp
        let value
        switch (el.type) {
          case 'checkbox':
            value = el.checked ? 1 : 0
            break
          case 'range':
          case 'text':
          case 'select-one':
            value = el.value
            break
          case 'button':
          case 'submit':
            value = '1'
            break
          default:
            return
        }

        const query = `${baseHost}/Set?type=1&reg=${el.id}&val=${value}`

        fetch(query)
          .then(response => {
            if ((el.id === 'code_secu') || (el.id === 'coche_secu')) {
              setTimeout(() => { get_value('codeR_secu'); }, 1000);
            }
            if ((el.id === 'vacances') || (el.id === 'planning')
            || (el.id === 'co_fi') || (el.id === 'cons_fixe')
            || (el.id === 'MMC')  || (el.id === 'fo_jus'))
            {
              setTimeout(() => { get_value(el.id); }, 1000);
            }
            if (el.id === 'planning') {
              setTimeout(() => { get_value('cons_fixe'); }, 1000);
            }
            if (el.id === 'cons_fixe') {
              setTimeout(() => { get_value('planning'); }, 1000);
            }
            

          })
      }

      // read initial values : requete_status
      const element = document.getElementById('nb_capteurs');
      Nombre_capteurs = element.value ;

      console.log ("Nb capteur DS18B20:"+Nombre_capteurs);


      function fetchStatusData()
      {
        fetch(`${baseHost}/status?type=${maj}`)
          .then(response => response.json())
          .then(state => updatePageValues(state))

          document
            .querySelectorAll('.default-action, .temperature span')
            .forEach(el => {
                el.onchange = () => updateConfig(el)
            })
      }
      window.fetchStatusData = fetchStatusData;

      setTimeout(() => { fetchStatusData(); } , 1000);

      function updatePageValues(state) {
        document
          .querySelectorAll('.default-action, .temperature span')  
          .forEach(el => {
            if (state[el.id] !== undefined) {
              let value = state[el.id];
              if (scale[el.id]) {
                value = (value / scale[el.id]).toFixed(1);
              }
              updateValue(el, value, false)
            }
          })

        if (state.cons_fixe !== undefined) {
            VisibiliteGroup("co_fi-group", Number(state.cons_fixe));
        }
        if (state.LRTT !== undefined) {
            VisibiliteGroup("Tint-mess-group", Number(state.LRTT) > 60);
        }
        if (state.vacances !== undefined) {
            VisibiliteGroup("vac-d-group", Number(state.vacances));
        }
        if (state.batSI !== undefined) {
            VisibiliteGroup("batS-group", Number(state.batSI));
        }

        if (state.fo_jus !== undefined) {
            VisibiliteGroup("jusque-group", parseInt(state.fo_jus));
        }

        for (var j =0; j<6; j++)
        {
          for (var i =0; i<NB_Val_Graph; i++)
          {
            let name = "T"+j+i;
            if (state[name] !== undefined) {
              graphique[j][i]=state[name];
            }
          }
        }
        for (var i =0; i<NB_erreurs; i++)
        {
          let name = "E"+i;
          if (state[name] !== undefined) {
            val2 = state[name].split(" ");
            erreur_code[i] = parseInt(val2[0]); 
            erreur_valeur[i] = parseInt(val2[1]); 
            erreur_val2[i] = parseInt(val2[2]); 
            erreur_jour[i] = parseInt(val2[3]); 
            erreur_heure[i] = parseInt(val2[4]); 
            erreur_minute[i] = parseInt(val2[5]); 
          }
        }
        
        for (var i = 0; i < 3; i++) {
          let name = "P" + i;
          if (state[name] !== undefined) {
            let vals = state[name].split(" ");
            for(let f=0; f<5; f++) {
                let el = document.getElementById("P"+i+"_"+f);
                if (el) {
                  // Pour les consignes (index 3 et 4) : div par 10
                  if (f === 3 || f === 4) {
                    el.value = (parseFloat(vals[f]) / 10).toFixed(1);
                  } 
                  // Pour les horaires (index 0 et 1), on convertit les pas de 10 min en HHhMM
                  else if (f === 0 || f === 1) {
                    el.value = stepsToTime(vals[f]);
                  }
                  else {
                    el.value = vals[f];
                  }
                }
            }
          }
        }

        function updatePlanning(pIdx, fieldIdx, value) {
          const reg = `P${pIdx}_${fieldIdx}`;
          let valToSend = value;
          // consignes (ind 3 et 4), on mult par 10
          if (fieldIdx === 3 || fieldIdx === 4) {
            valToSend = Math.round(parseFloat(value.toString().replace(',', '.')) * 10);
          }
          // horaires (ind 0 et 1), on convertit le format HHhMM en pas de 10 min
          else if (fieldIdx === 0 || fieldIdx === 1) {
            valToSend = timeToSteps(value);
          }
          const query = `${document.location.origin}/Set?type=1&reg=${reg}&val=${valToSend}`;
          fetch(query)
            .then(response => {
              // Relire la valeur exacte depuis l'ESP
              setTimeout(() => {
                const getQuery = `${document.location.origin}/Get?type=1&reg=${reg}`;
                fetch(getQuery)
                  .then(res => res.json())
                  .then(state => {
                    if (state.val !== undefined) {
                      let el = document.getElementById(reg);
                      if (fieldIdx === 3 || fieldIdx === 4) {
                        el.value = (parseFloat(state.val) / 10).toFixed(1);
                      } else if (fieldIdx === 0 || fieldIdx === 1) {
                        el.value = stepsToTime(state.val);
                      } else {
                        el.value = state.val;
                      }
                    }
                  });
              }, 1000);
            });
        }
        window.updatePlanning = updatePlanning;
 
        document.getElementById('erreurs_histo').innerHTML = ""; 
        for (i=0; i<NB_erreurs; i++)
        {
          if (erreur_code[i])
            document.getElementById('erreurs_histo').innerHTML += erreur_jour[i] +'-'+ erreur_heure[i]+'h'+ erreur_minute[i]+ ' code:' + erreur_code[i]+' : '+ erreur_valeur[i] +' - '+ erreur_val2[i] + ' <br>';
        }

        if (!maj)
        {
          dessine_graphe("schema", 0, 100, ["#00f808", "#0098f8", "#f84200"]);
          dessine_graphe("schema2", 3, 100, ["#00f808", "#0098f8", "#f84200"]);
        }
        maj=1; // => pas de graphique
      }
    })

    function dessine_graphe(canvasId, dataOffset, divider, colors)
    {
      var zone_dessin = document.getElementById(canvasId);
      var graphe= zone_dessin.getContext("2d");
      var compteur=0;
      var vert_min = 35;
      var vert_max = -10;
      
      // Calcul min/max
      for (j=0; j<3; j++) {
        for (i=0; i<NB_Val_Graph; i++) {
          let val = f(j, i);
          if (val < vert_min) vert_min = val;
          if (val > vert_max) vert_max = val;
        }
      }
      
      if (vert_min < -10) vert_min = -10;
      if (vert_max > 35) vert_max = 35;
      vert_min = Math.floor(vert_min);
      vert_max = Math.ceil(vert_max);
          
      graphe.lineWidth = 2;

      for (num=0; num<3; num++) {
        graphe.strokeStyle = colors[num];
        graphe.beginPath();
        graphe.moveTo(zone_dessin.width, (vert_max - f(num, 0)) * zone_dessin.height / (vert_max - vert_min));
        
        compteur = 1;
        while(compteur < NB_Val_Graph) {
          graphe.lineTo(zone_dessin.width - (compteur * zone_dessin.width / NB_Val_Graph), 
                       (vert_max - f(num, compteur)) * zone_dessin.height / (vert_max - vert_min));
          compteur = (compteur + 1);
        }           
        graphe.stroke();
      }

      function f(num, x) {
        var y = (graphique[num + dataOffset][x]) / divider;
        if ((num == 1))  y = y/5; // pour les humidite, on divise par 50
        return (y);
      }

      // Axes
      graphe.beginPath();
      graphe.lineWidth = "1";
      graphe.strokeStyle = "white";
      graphe.moveTo(0, zone_dessin.height / 2);
      graphe.lineTo(zone_dessin.width, zone_dessin.height / 2);  // horiz
      graphe.moveTo(0, zone_dessin.height);   // vertical
      graphe.lineTo(0, 0);
      graphe.moveTo(0, 0);
      graphe.lineTo(5, 5);

      // Tirets
      compteur = 0;
      var x = 0;
      while(compteur < 12) {
        graphe.moveTo(x, zone_dessin.height / 2 - 5);
        graphe.lineTo(x, zone_dessin.height / 2 + 5);
        compteur = (compteur + 1);
        x = x + zone_dessin.width / 12;
      } 
      graphe.stroke();
      
      graphe.fillStyle = "white";
      graphe.fillText(vert_min, 5, -8 + zone_dessin.height);
      graphe.fillText(vert_max, 5, 8);
    }


    // Action type 5:

    document.getElementById('ActionBtn').addEventListener('click', () => {
      const reg   = document.getElementById('Action_reg').value.trim();
      const value = parseFloat(document.getElementById('Action_data').value);
      const baseHost = document.location.origin;
      const query = `${baseHost}/Set?type=5&reg=${encodeURIComponent(reg)}&val=${value}`;

      const resultA = document.getElementById('actionResult');
      resultA.textContent = 'Envoi en cours…';

      fetch(query)
         .then(response => response.text())
         .then(text => {
            resultA.textContent = text;
         })
         .catch(err => {
          resultA.textContent = 'Erreur lors de l’envoi';
          console.error('Erreur fetch :', err);
         });
    });

    document.getElementById('loadLogsBtn').addEventListener('click', () => {
    const count = document.getElementById('countInput').value;
    const url = `/GetLogs?count=${count}`;

    fetch(url)
      .then(response => response.arrayBuffer())
      .then(buffer => {
        const byteArray = new Uint8Array(buffer);
        displayLogsBinary(byteArray);
      })
      .catch(err => {
        alert('Erreur lors de la récupération des logs : ' + err);
      });
  });

  function padLeft(str, length) {
    return str.toString().padStart(length, ' ');
  }

  function displayLogsBinary(buffer) {
    const LOG_SIZE = 16;

    const logCount = buffer.length / LOG_SIZE;
    let html = "";

    for (let i = 0; i < logCount; i++) {
      const offset = i * LOG_SIZE;

      const timestamp = buffer[offset] |
                        (buffer[offset + 1] << 8) |
                        (buffer[offset + 2] << 16) |
                        (buffer[offset + 3] << 24);
      const code = buffer[offset + 4];
      const codeChar = String.fromCharCode(code);
      const c1 = buffer[offset + 5];
      const c2 = buffer[offset + 6];
      const c3 = buffer[offset + 7];
      const messageBytes = buffer.slice(offset + 8, offset + 16);
      const message = new TextDecoder('utf-8').decode(messageBytes).replace(/\0/g, '');

      const date = new Date(timestamp * 1000);
      const pad = n => n.toString().padStart(2, '0');
      const dateStr = `${pad(date.getDate())}/${pad(date.getMonth() + 1)}/${date.getFullYear()} ` +
                      `${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`;

      let logLine = `${i + 1}:${dateStr} :${codeChar} ${padLeft(c1, 2)} ${padLeft(c2, 2)} ${padLeft(c3, 2)} "${message}"<br>`;
      html += logLine;
    }

    document.getElementById("log_histo").innerHTML = html;
  }

    </script>
  </body>
  </html>)rawliteral";

// Favicon : 
// utiliser d'abord https://www.icoconverter.com/
// puis https://tomeko.net pour avoir les codes ascii

 //0x00, 0x00, 0x01, 0x00, 0x01, 0x00,  // En-tête fichier : 1 image 

  /*Largeur (1 octet) : 32 pour une image de 32x32 px.
  Hauteur (1 octet) : 32 pour une image de 32x32 px.
  Nombre de couleurs (1 octet) : 0 (pour une image monochrome, cela signifie que la profondeur de couleur est indiquée par le champ suivant).
  Réservé (1 octet) : 0.
  Plan de couleurs (2 octets) : 0.
  Profondeur de couleur (2 octets) : 1 bit par pixel pour une image monochrome. */
  //0x10, 0x10, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, // en-tete-image

  //Taille du fichier image (4 octets) : JPG:500 octets  ICO:1200 octets (taille de l'image binaire).
  //Offset de l'image (4 octets) : Où l'image commence dans le fichier (après l'en-tête des images).

  const uint8_t  favicon[] PROGMEM = {
0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x68, 0x05, 
0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xC3, 0x0E, 
0x00, 0x00, 0xC3, 0x0E, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xFE, 0xCC, 0x55, 0x00, 0xFD, 0xCB, 0x54, 0x00, 0xFC, 0xC9, 0x51, 0x00, 0xFA, 0xC7, 
0x4D, 0x00, 0xF8, 0xC4, 0x4A, 0x00, 0xF7, 0xC2, 0x46, 0x00, 0xF5, 0xC0, 0x42, 0x00, 0xF3, 0xBC, 
0x3C, 0x00, 0xF1, 0xBA, 0x38, 0x00, 0xEF, 0xB8, 0x36, 0x00, 0xEF, 0xB7, 0x34, 0x00, 0xFF, 0xD0, 
0x5B, 0x00, 0xF3, 0xBD, 0x3E, 0x00, 0xED, 0xB5, 0x30, 0x00, 0xEC, 0xB3, 0x2F, 0x00, 0xF5, 0xBF, 
0x40, 0x00, 0xEA, 0xB1, 0x2B, 0x00, 0xE8, 0xAF, 0x27, 0x00, 0xE9, 0xAF, 0x28, 0x00, 0xF8, 0xCB, 
0x60, 0x00, 0xF5, 0xC7, 0x5B, 0x00, 0xF3, 0xC4, 0x55, 0x00, 0xF0, 0xBC, 0x40, 0x00, 0xE6, 0xAC, 
0x24, 0x00, 0xE3, 0xA8, 0x1D, 0x00, 0xF8, 0xCC, 0x63, 0x00, 0xFD, 0xF4, 0xDF, 0x00, 0xFE, 0xF9, 
0xEE, 0x00, 0xFE, 0xFA, 0xF1, 0x00, 0xFA, 0xEC, 0xCA, 0x00, 0xF0, 0xC6, 0x62, 0x00, 0xE5, 0xAA, 
0x1F, 0x00, 0xF7, 0xCB, 0x63, 0x00, 0xF9, 0xF5, 0xED, 0x00, 0xE5, 0xD1, 0xA4, 0x00, 0xEF, 0xDF, 
0xBA, 0x00, 0xFE, 0xFE, 0xFD, 0x00, 0xF5, 0xE9, 0xCE, 0x00, 0xE0, 0xAD, 0x34, 0x00, 0xE6, 0xAC, 
0x22, 0x00, 0xE3, 0xA7, 0x1C, 0x00, 0xE1, 0xA5, 0x19, 0x00, 0xF4, 0xEB, 0xD7, 0x00, 0xCE, 0xA0, 
0x32, 0x00, 0xDD, 0xAE, 0x41, 0x00, 0xFB, 0xF4, 0xE4, 0x00, 0xCA, 0xA1, 0x40, 0x00, 0xD9, 0xA1, 
0x1C, 0x00, 0xDF, 0xA3, 0x14, 0x00, 0xF4, 0xC7, 0x5C, 0x00, 0xFA, 0xF5, 0xE9, 0x00, 0xEA, 0xCF, 
0x8F, 0x00, 0xF3, 0xDC, 0xA7, 0x00, 0xFE, 0xFD, 0xFA, 0x00, 0xF1, 0xE7, 0xD0, 0x00, 0xC2, 0x95, 
0x2C, 0x00, 0xC4, 0x90, 0x17, 0x00, 0xD9, 0x9F, 0x17, 0x00, 0xDC, 0x9F, 0x10, 0x00, 0xF2, 0xC5, 
0x59, 0x00, 0xFB, 0xF9, 0xF5, 0x00, 0xFA, 0xF8, 0xF2, 0x00, 0xD0, 0xAD, 0x5B, 0x00, 0xBF, 0x8D, 
0x16, 0x00, 0xC9, 0x92, 0x12, 0x00, 0xD8, 0x9C, 0x10, 0x00, 0xDC, 0x9E, 0x0D, 0x00, 0xF1, 0xC3, 
0x56, 0x00, 0xF4, 0xED, 0xDE, 0x00, 0xCA, 0xA8, 0x59, 0x00, 0xC7, 0xA0, 0x46, 0x00, 0xC3, 0x8F, 
0x15, 0x00, 0xC6, 0x90, 0x12, 0x00, 0xC9, 0x91, 0x0F, 0x00, 0xCE, 0x94, 0x0B, 0x00, 0xD6, 0x99, 
0x09, 0x00, 0xEF, 0xC1, 0x53, 0x00, 0xF1, 0xE9, 0xD5, 0x00, 0xBD, 0x91, 0x29, 0x00, 0xBB, 0x8A, 
0x16, 0x00, 0xCC, 0x93, 0x0B, 0x00, 0xCF, 0x94, 0x08, 0x00, 0xD1, 0x94, 0x06, 0x00, 0xEC, 0xB3, 
0x2D, 0x00, 0xED, 0xBD, 0x4C, 0x00, 0xF8, 0xEF, 0xDC, 0x00, 0xED, 0xE0, 0xC3, 0x00, 0xC1, 0x93, 
0x28, 0x00, 0xD4, 0x95, 0x02, 0x00, 0xD6, 0xA9, 0x3F, 0x00, 0xC5, 0x9A, 0x35, 0x00, 0xC0, 0x8F, 
0x1A, 0x00, 0xE2, 0xA7, 0x1D, 0x00, 0xCD, 0x97, 0x17, 0x00, 0xD1, 0x98, 0x13, 0x00, 0xD8, 0x95, 
0x00, 0x00, 0xD1, 0x96, 0x0C, 0x00, 0xCE, 0x93, 0x08, 0x00, 0xD3, 0x95, 0x03, 0x00, 0xFF, 0xFF, 
0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x20, 0x20, 0x19, 0x2A, 0x31, 0x3B, 0x61, 0x62, 0x53, 0x63, 0x59, 0x59, 0x00, 0x00, 0x12, 0x12, 
0x13, 0x18, 0x20, 0x29, 0x2A, 0x5F, 0x4A, 0x51, 0x52, 0x53, 0x59, 0x59, 0x59, 0x60, 0x0F, 0x0E, 
0x11, 0x12, 0x18, 0x5D, 0x5E, 0x48, 0x49, 0x4A, 0x51, 0x52, 0x53, 0x59, 0x59, 0x59, 0x09, 0x0E, 
0x0E, 0x13, 0x11, 0x5A, 0x5B, 0x5C, 0x48, 0x49, 0x4A, 0x51, 0x52, 0x53, 0x59, 0x59, 0x09, 0x0B, 
0x0E, 0x54, 0x55, 0x56, 0x57, 0x58, 0x40, 0x48, 0x49, 0x4A, 0x51, 0x52, 0x53, 0x59, 0x08, 0x09, 
0x0B, 0x0E, 0x4D, 0x1D, 0x4E, 0x4F, 0x50, 0x40, 0x48, 0x49, 0x4A, 0x51, 0x52, 0x53, 0x07, 0x0D, 
0x09, 0x0B, 0x44, 0x1D, 0x45, 0x46, 0x47, 0x38, 0x40, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x06, 0x07, 
0x0D, 0x09, 0x3C, 0x1C, 0x25, 0x3D, 0x3E, 0x37, 0x3F, 0x40, 0x40, 0x41, 0x42, 0x43, 0x05, 0x06, 
0x07, 0x08, 0x32, 0x1D, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x31, 0x3B, 0x04, 0x05, 
0x06, 0x10, 0x21, 0x1D, 0x2B, 0x2C, 0x2D, 0x2E, 0x22, 0x2F, 0x30, 0x19, 0x2A, 0x31, 0x03, 0x04, 
0x05, 0x06, 0x21, 0x1D, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x20, 0x29, 0x2A, 0x03, 0x03, 
0x04, 0x05, 0x1A, 0x1B, 0x1C, 0x1D, 0x1C, 0x1E, 0x1F, 0x13, 0x12, 0x18, 0x20, 0x19, 0x01, 0x02, 
0x03, 0x04, 0x04, 0x14, 0x15, 0x15, 0x16, 0x17, 0x0E, 0x0F, 0x11, 0x12, 0x18, 0x19, 0x01, 0x01, 
0x01, 0x03, 0x04, 0x05, 0x06, 0x10, 0x08, 0x09, 0x0B, 0x0E, 0x0E, 0x11, 0x12, 0x13, 0x0C, 0x01, 
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0D, 0x09, 0x0B, 0x0E, 0x0F, 0x0E, 0x0E, 0x00, 0x00, 
0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
