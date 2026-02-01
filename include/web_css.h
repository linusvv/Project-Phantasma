#ifndef WEB_CSS_H
#define WEB_CSS_H

const char PAGE_CSS[] PROGMEM = R"rawliteral(
<style>
    body { font-family: 'Segoe UI', Arial; text-align: center; background: #eef2f3; margin: 0; padding-bottom: 50px; }
    h2 { color: #2c3e50; margin-top: 20px; }
    button { padding: 15px 25px; font-size: 16px; margin: 10px; border: none; border-radius: 5px; cursor: pointer; background: #2980b9; color: white; transition: 0.3s; }
    button:hover { background: #3498db; }
    button.secondary { background: #95a5a6; }
    button.active { background: #27ae60; }
    
    .container { max-width: 600px; margin: 0 auto; display: none; }
    .visible { display: block; }
    
    .data-box {
      background: #fff; padding: 15px; margin: 15px auto;
      width: 90%; border-radius: 10px;
      box-shadow: 0 4px 10px rgba(0,0,0,0.1);
      display: grid; grid-template-columns: 1fr 1fr; gap: 10px;
    }
    .data-item { font-size: 18px; font-weight: bold; color: #2980b9; }
    .data-label { font-size: 12px; color: #7f8c8d; display: block; }

    .slider-card {
      background: white; margin: 10px auto; padding: 15px;
      width: 90%; border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.05); text-align: left;
    }
    input[type=range] { width: 100%; height: 20px; accent-color: #2980b9; cursor: pointer; }
    .label-row { display: flex; justify-content: space-between; font-weight: 600; color: #34495e; margin-bottom: 5px; }

    .input-group { display: flex; gap: 10px; justify-content: center; margin: 10px 0; }
    .input-wrapper { display: flex; flex-direction: column; width: 60px; }
    .input-wrapper input { padding: 5px; text-align: center; border: 1px solid #ccc; border-radius: 4px; }
    
    .error { color: #c0392b; font-weight: bold; margin: 10px auto; display: none; border: 2px solid #c0392b; padding: 10px; border-radius: 5px; background: #fadbd8; width: 80%; }
    .rec-btn { background: #e74c3c; }
    .rec-active { animation: pulse 1.5s infinite; background: #c0392b !important; border: 2px solid white; }
    @keyframes pulse { 0% { transform: scale(1); } 50% { transform: scale(1.05); } 100% { transform: scale(1); } }
    
    .mic-btn { font-size: 24px; border-radius: 50%; width: 60px; height: 60px; padding: 0; display: flex; align-items: center; justify-content: center; margin: 0 auto; background: #f39c12; -webkit-user-select: none; user-select: none; -webkit-touch-callout: none; touch-action: none; }
    .mic-active { background: #e74c3c; animation: pulse 1s infinite; }
    
    .file-label { display: block; width: 100%; padding: 12px; background: #95a5a6; color: white; border-radius: 5px; cursor: pointer; text-align: center; margin-bottom: 10px; max-width: 250px; margin-left: auto; margin-right: auto; box-sizing: border-box; }
    .file-label:hover { background: #7f8c8d; }
</style>
)rawliteral";

#endif
