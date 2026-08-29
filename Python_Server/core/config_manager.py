import os
import yaml

DEFAULT_CONFIG = {
    "camera": {
        "source": 0,
        "width": 640,
        "height": 480,
        "fps": 15,
        "analyze_interval_sec": 10
    },
    "gemini": {
        "enabled": True,
        "api_key": "YOUR_GEMINI_API_KEY",
        "model": "gemini-2.0-flash",
        "temperature": 0.2
    },
    "telegram": {
        "enabled": False,
        "bot_token": "",
        "chat_id": "",
        "cooldown_minutes": 10
    },
    "thingsboard": {
        "enabled": False,
        "host": "demo.thingsboard.io",
        "port": 1883,
        "access_token": ""
    },
    "esp32_lan": {
        "enabled": True,
        "base_url": "http://beca.local",
        "timeout_sec": 2.0
    },
    "aquarium": {
        "total_fish": 10
    },
    "web": {
        "host": "0.0.0.0",
        "port": 5000
    }
}

class ConfigManager:
    def __init__(self, config_path="config.yaml"):
        # Tìm config.yaml cùng thư mục với run_server.py
        base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        if not os.path.isabs(config_path):
            self.config_path = os.path.join(base_dir, config_path)
        else:
            self.config_path = config_path
            
        self.config = self.load_config()

    def load_config(self):
        if not os.path.exists(self.config_path):
            print(f"[CONFIG] Khong tim thay {self.config_path}, su dung cau hinh mac dinh.")
            return DEFAULT_CONFIG.copy()

        try:
            with open(self.config_path, "r", encoding="utf-8") as f:
                loaded = yaml.safe_load(f)
                if not loaded:
                    return DEFAULT_CONFIG.copy()
                
                config = DEFAULT_CONFIG.copy()
                for k, v in loaded.items():
                    if isinstance(v, dict) and k in config:
                        config[k].update(v)
                    else:
                        config[k] = v
                return config
        except Exception as e:
            print(f"[CONFIG ERROR] Loi doc {self.config_path}: {e}")
            return DEFAULT_CONFIG.copy()

    def get(self, section, key=None, default=None):
        sec_val = self.config.get(section, {})
        if key is None:
            return sec_val
        if isinstance(sec_val, dict):
            return sec_val.get(key, default)
        return default

    def set(self, section, key, value):
        if section not in self.config:
            self.config[section] = {}
        self.config[section][key] = value

    def save_config(self):
        try:
            with open(self.config_path, "w", encoding="utf-8") as f:
                yaml.dump(self.config, f, allow_unicode=True, default_flow_style=False)
            print(f"[CONFIG] Da luu cau hinh moi vao {self.config_path}")
            return True
        except Exception as e:
            print(f"[CONFIG ERROR] Loi luu {self.config_path}: {e}")
            return False
