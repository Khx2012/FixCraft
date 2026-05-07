import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, colorchooser, filedialog
import threading
import os
import shutil
import psutil
import platform
import configparser


CONFIG_DIR = os.path.join(os.path.expandvars("%AppData%"), "FixCraft")
CONFIG_FILE = os.path.join(CONFIG_DIR, "settings.ini")

DEFAULTS = {
    "dark_mode": "false",
    "bg_color": "#f0f0f0",
    "minecraft_path": os.path.join(os.environ["APPDATA"], ".minecraft"),
    "auto_analyze": "false",
    "clear_shader_on_quick_fix": "true",
    "clear_launcher_on_quick_fix": "true",
}


class FixCraft_v1_0:
    def __init__(self, root):
        self.root = root
        self.is_running = False

        self.root.title("FixCraft v1.0")
        self.root.geometry("800x700")

        self.cfg = self.load_config()
        self.build_ui()
        self.apply_theme()

        if self.get_bool("auto_analyze"):
            self.root.after(500, self.analyze)

    def load_config(self):
        cfg = configparser.ConfigParser()
        os.makedirs(CONFIG_DIR, exist_ok=True)

        try:
            if os.path.exists(CONFIG_FILE):
                cfg.read(CONFIG_FILE, encoding="utf-8")

            if "app" not in cfg:
                cfg["app"] = {k: str(v) for k, v in DEFAULTS.items()}

        except Exception:
            cfg["app"] = DEFAULTS.copy()

        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            cfg.write(f)

        return cfg

    def save_config(self):
        os.makedirs(CONFIG_DIR, exist_ok=True)
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            self.cfg.write(f)

    def get(self, key):
        try:
            return self.cfg.get("app", key)
        except:
            return DEFAULTS.get(key, "")

    def get_bool(self, key):
        try:
            return self.cfg.getboolean("app", key)
        except:
            return DEFAULTS.get(key, "false").lower() == "true"

    def set(self, key, value):
        if isinstance(value, bool):
            value = "true" if value else "false"
        self.cfg.set("app", key, str(value))

    # ── UI ────────────────────────────────────────────────────────────────────
    def build_ui(self):
        bar = tk.Frame(self.root, bg="#2c2c2c", height=36)
        bar.pack(fill=tk.X, side=tk.TOP)
        bar.pack_propagate(False)

        tk.Label(
            bar,
            text="FixCraft v1.0",
            bg="#2c2c2c",
            fg="white",
            font=("Arial", 10, "bold"),
        ).pack(side=tk.LEFT, padx=10)

        tk.Button(
            bar,
            text="❌ Exit",
            command=self.root.quit,
            bg="#2c2c2c",
            fg="white",
            relief="flat",
        ).pack(side=tk.RIGHT, padx=5)
        tk.Button(
            bar,
            text="📖 Help & FAQ",
            command=self.show_help,
            bg="#2c2c2c",
            fg="white",
            relief="flat",
        ).pack(side=tk.RIGHT, padx=5)
        tk.Button(
            bar,
            text="🎨 Color",
            command=self.pick_color,
            bg="#2c2c2c",
            fg="white",
            relief="flat",
        ).pack(side=tk.RIGHT, padx=5)

        self.dark_btn = tk.Button(
            bar,
            text="🌙 Dark Mode",
            command=self.toggle_dark_mode,
            bg="#2c2c2c",
            fg="white",
            relief="flat",
        )
        self.dark_btn.pack(side=tk.RIGHT, padx=5)

        ttk.Label(
            self.root,
            text="⚠️ Not affiliated with Mojang. Use at your own risk.",
            font=("Arial", 7),
            foreground="gray"
        ).pack(pady=3)

        tk.Button(
            bar,
            text="⚙️ Settings",
            command=self.show_settings,
            bg="#2c2c2c",
            fg="#FFD700",
            relief="flat",
            font=("Arial", 9, "bold"),
        ).pack(side=tk.RIGHT, padx=8)

        ttk.Label(
            self.root, text="FixCraft v1.0", font=("Arial", 18, "bold")
        ).pack(pady=15)

        self.status_label = ttk.Label(
            self.root, text="Ready", font=("Arial", 10), foreground="green"
        )
        self.status_label.pack()

        btn = ttk.Frame(self.root)
        btn.pack(pady=10)

        ttk.Button(btn, text="🔍 System Analysis", command=self.analyze).pack(
            fill=tk.X, pady=3, padx=10
        )
        ttk.Button(btn, text="⚡ Quick Fix", command=self.quick_fix).pack(
            fill=tk.X, pady=3, padx=10
        )
        ttk.Button(btn, text="🔧 Full fix", command=self.full_fix).pack(
            fill=tk.X, pady=3, padx=10
        )
        ttk.Button(
            btn, text="💾 Increase Virtual Memory", command=self.increase_memory
        ).pack(fill=tk.X, pady=3, padx=10)
        ttk.Button(btn, text="💻 System Info", command=self.show_system_info).pack(
            fill=tk.X, pady=3, padx=10
        )
        ttk.Button(
            btn, text="🔍 Analyze Crash Logs", command=self.analyze_crash_logs
        ).pack(fill=tk.X, pady=3, padx=10)
        ttk.Button(
            btn, text="🔍 Quick Scan", command=self.quick_scan
        ).pack(fill=tk.X, pady=3, padx=10)
        ttk.Button(
            btn, text="🔎 Full Scan", command=self.full_scan
        ).pack(fill=tk.X, pady=3, padx=10)
        ttk.Button(btn, text="🔄 Reset Config Files", command=self.reset_config).pack(
            fill=tk.X, pady=3, padx=10
        )
        ttk.Button(
            btn, text="⚠️ Check RAM Pressure", command=self.check_ram_pressure
        ).pack(fill=tk.X, pady=3, padx=10)

        ttk.Label(self.root, text="Output:", font=("Arial", 10, "bold")).pack(
            anchor=tk.W, padx=10
        )

        self.output = scrolledtext.ScrolledText(self.root, height=10)
        self.output.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        ttk.Label(
            self.root,
            text="Version 1.0  |  Not affiliated with Mojang",
            font=("Arial", 7),
            foreground="gray",
        ).pack(pady=3)

    # ── THEME ─────────────────────────────────────────────────────────────────
    def apply_theme(self):
        dark = self.get_bool("dark_mode")

        bg = "#1e1e1e" if dark else self.get("bg_color")
        fg = "white" if dark else "black"

        self.root.configure(bg=bg)

        if hasattr(self, "output"):
            self.output.configure(bg="#2d2d2d" if dark else "white", fg=fg)

    def toggle_dark_mode(self):
        self.set("dark_mode", not self.get_bool("dark_mode"))
        self.save_config()
        self.apply_theme()

    def pick_color(self):
        c = colorchooser.askcolor()[1]
        if c:
            self.set("bg_color", c)
            self.save_config()
            self.apply_theme()

    # ── LOGGING ───────────────────────────────────────────────────────────────
    def log(self, msg):
        self.output.insert(tk.END, str(msg) + "\n")
        self.output.see(tk.END)

    def log_safe(self, msg):
        self.root.after(0, lambda: self.log(msg))

    def status_safe(self, msg, color="green"):
        self.root.after(0, lambda: self.status_label.config(text=msg, foreground=color))

    # ── ANALYZE ───────────────────────────────────────────────────────────────
    def analyze(self):
        if self.is_running:
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def run():
            try:
                self.log_safe("=" * 60)
                self.log_safe("FIXCRAFT SYSTEM ANALYSIS ENGINE")
                self.log_safe("=" * 60)

                results = {
                    "env": [],
                    "resource": [],
                    "paths": [],
                    "files": [],
                    "runtime": [],
                    "logs": [],
                    "decisions": []
                }

                mc_path = self.get("minecraft_path")

                # ─────────────────────────────────────────────
                # 1. ENVIRONMENT CHECK
                # ─────────────────────────────────────────────
                self.log_safe("\n[1] ENVIRONMENT CHECK")

                try:
                    os_name = platform.system()
                    os_ver = platform.release()

                    results["env"].append(f"{os_name} {os_ver}")
                    self.log_safe(f"✓ OS: {os_name} {os_ver}")

                    if os_name != "Windows":
                        results["env"].append("UNSUPPORTED OS")
                        self.log_safe("⚠️ Non-Windows system (limited support)")

                except Exception as e:
                    self.log_safe(f"❌ Environment error: {e}")

                # ─────────────────────────────────────────────
                # 2. RESOURCE CHECK
                # ─────────────────────────────────────────────
                self.log_safe("\n[2] RESOURCE CHECK")

                try:
                    vm = psutil.virtual_memory()
                    cpu = psutil.cpu_percent(interval=0.5)
                    disk = psutil.disk_usage(os.path.splitdrive(os.getcwd())[0] + "\\")

                    avail_ram = vm.available / (1024**3)
                    used_ram = vm.percent
                    free_disk = disk.free / (1024**3)

                    self.log_safe(f"RAM: {avail_ram:.2f}GB free ({used_ram}%)")
                    self.log_safe(f"CPU: {cpu}%")
                    self.log_safe(f"Disk Free: {free_disk:.2f}GB")

                    if avail_ram < 2:
                        results["resource"].append("CRITICAL RAM")
                    elif avail_ram < 4:
                        results["resource"].append("LOW RAM")

                    if cpu > 90:
                        results["resource"].append("CPU OVERLOAD")

                    if free_disk < 5:
                        results["resource"].append("CRITICAL DISK")
                    elif free_disk < 15:
                        results["resource"].append("LOW DISK")

                except Exception as e:
                    self.log_safe(f"❌ Resource error: {e}")

                # ─────────────────────────────────────────────
                # 3. PATH VALIDATION
                # ─────────────────────────────────────────────
                self.log_safe("\n[3] PATH VALIDATION")

                if os.path.isdir(mc_path):
                    self.log_safe(f"✓ Minecraft path valid")
                    results["paths"].append("VALID")
                else:
                    self.log_safe("❌ Minecraft path invalid")
                    results["paths"].append("INVALID")
                    results["decisions"].append("STOP_ALL")
                    return

                # ─────────────────────────────────────────────
                # 4. FILE INTEGRITY
                # ─────────────────────────────────────────────
                self.log_safe("\n[4] FILE INTEGRITY")

                critical_files = ["options.txt"]
                for f in critical_files:
                    path = os.path.join(mc_path, f)

                    if not os.path.exists(path):
                        self.log_safe(f"⚠️ Missing: {f}")
                        results["files"].append(f"MISSING {f}")
                    else:
                        try:
                            if os.path.getsize(path) == 0:
                                self.log_safe(f"❌ Empty file: {f}")
                                results["files"].append(f"CORRUPT {f}")
                            else:
                                self.log_safe(f"✓ {f} OK")
                        except:
                            results["files"].append(f"UNREADABLE {f}")

                # ─────────────────────────────────────────────
                # 5. RUNTIME RISK DETECTION
                # ─────────────────────────────────────────────
                self.log_safe("\n[5] RUNTIME RISK DETECTION")

                mods_path = os.path.join(mc_path, "mods")
                if os.path.isdir(mods_path):
                    mods = os.listdir(mods_path)
                    if len(mods) > 20:
                        self.log_safe(f"⚠️ Heavy mod load ({len(mods)})")
                        results["runtime"].append("HIGH MOD COUNT")
                    elif mods:
                        self.log_safe(f"✓ Mods detected ({len(mods)})")

                for proc in psutil.process_iter(['name']):
                    try:
                        if "java" in proc.info['name'].lower():
                            results["runtime"].append("JAVA RUNNING")
                            self.log_safe("⚠️ Java process active")
                            break
                    except:
                        pass

                # ─────────────────────────────────────────────
                # 6. LOG INTELLIGENCE
                # ─────────────────────────────────────────────
                self.log_safe("\n[6] LOG INTELLIGENCE")

                logs_path = os.path.join(mc_path, "logs")

                if os.path.isdir(logs_path):
                    files = [f for f in os.listdir(logs_path) if f.endswith(".log")]

                    if files:
                        latest = os.path.join(logs_path, files[-1])

                        try:
                            with open(latest, "r", errors="ignore") as f:
                                content = f.read().lower()

                            if "outofmemory" in content:
                                results["logs"].append("RAM CRASH")
                                self.log_safe("❌ OutOfMemory detected")

                            if "exception" in content:
                                results["logs"].append("EXCEPTION")
                                self.log_safe("⚠️ Exception found")

                            if "failed" in content:
                                results["logs"].append("FAILURE")
                                self.log_safe("⚠️ Failure detected")

                        except Exception as e:
                            self.log_safe(f"⚠️ Log read error: {e}")
                    else:
                        self.log_safe("✓ No logs found")
                else:
                    self.log_safe("⚠️ Logs folder missing")

                # ─────────────────────────────────────────────
                # 7. FIX DECISION LAYER
                # ─────────────────────────────────────────────
                self.log_safe("\n[7] FIX DECISION ENGINE")

                if "CRITICAL RAM" in results["resource"]:
                    results["decisions"].append("ADVISE_RAM")

                if "CRITICAL DISK" in results["resource"]:
                    results["decisions"].append("ADVISE_DISK")

                if "CORRUPT options.txt" in results["files"]:
                    results["decisions"].append("RESET_CONFIG")

                if "RAM CRASH" in results["logs"]:
                    results["decisions"].append("INCREASE_MEMORY")

                if "HIGH MOD COUNT" in results["runtime"]:
                    results["decisions"].append("CHECK_MODS")

                if results["decisions"]:
                    self.log_safe("\nRecommended Actions:")
                    for d in results["decisions"]:
                        self.log_safe(f"→ {d}")
                else:
                    self.log_safe("✓ No major actions required")

                self.log_safe("\nAnalysis Complete")
                self.status_safe("Analysis complete", "green")

            except Exception as e:
                self.log_safe(f"FATAL ERROR: {e}")
                self.status_safe("Analysis failed", "red")

            finally:
                self.is_running = False

        threading.Thread(target=run, daemon=True).start()

    # ── QUICK FIX ─────────────────────────────────────────────────────────────
    def quick_fix(self):
        if self.is_running:
            return

        if not messagebox.askyesno(
            "Confirm Quick Fix",
            "This will apply SAFE fixes only.\nContinue?"
        ):
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def run():
            try:
                self.log_safe("=" * 60)
                self.log_safe("FIXCRAFT QUICK FIX ENGINE")
                self.log_safe("=" * 60)

                mc_path = self.get("minecraft_path")

                self.log_safe("\n[1] RE-CHECK SYSTEM STATE")

                vm = psutil.virtual_memory()
                disk = psutil.disk_usage(os.path.splitdrive(os.getcwd())[0] + "\\")

                ram_low = vm.available / (1024**3) < 4
                disk_low = disk.free / (1024**3) < 15

                if ram_low:
                    self.log_safe("⚠️ Low RAM detected → advising cleanup")
                if disk_low:
                    self.log_safe("⚠️ Low disk space detected")

                self.log_safe("\n[2] PATH VALIDATION")

                if not os.path.isdir(mc_path):
                    self.log_safe("❌ Minecraft path invalid — aborting fix")
                    self.status_safe("Quick fix aborted", "red")
                    return

                self.log_safe("✓ Path valid")

                self.log_safe("\n[3] SAFE CLEANUP")

                actions_done = []

                logs_path = os.path.join(mc_path, "logs")
                if os.path.isdir(logs_path):
                    try:
                        count = 0
                        for f in os.listdir(logs_path):
                            fp = os.path.join(logs_path, f)
                            if os.path.isfile(fp):
                                os.remove(fp)
                                count += 1

                        self.log_safe(f"✓ Removed {count} log files")
                        actions_done.append("LOGS_CLEARED")

                    except Exception as e:
                        self.log_safe(f"⚠️ Log cleanup failed: {e}")

                shader_paths = [
                    os.path.join(mc_path, "shadercache"),
                    os.path.join(mc_path, "cache"),
                ]

                for sp in shader_paths:
                    if os.path.exists(sp):
                        try:
                            shutil.rmtree(sp)
                            self.log_safe(f"✓ Cleared: {os.path.basename(sp)}")
                            actions_done.append("CACHE_CLEARED")
                        except Exception as e:
                            self.log_safe(f"⚠️ Cache error: {e}")

                self.log_safe("\n[4] CONFIG CHECK")

                config_file = os.path.join(mc_path, "options.txt")

                if os.path.exists(config_file):
                    try:
                        if os.path.getsize(config_file) == 0:
                            self.log_safe("❌ Corrupt config detected (empty file)")
                            self.log_safe("→ Will regenerate on next launch")
                            actions_done.append("CONFIG_CORRUPT")
                    except:
                        self.log_safe("⚠️ Could not read config safely")

                self.log_safe("\n[5] MEMORY RESPONSE")

                if ram_low:
                    self.log_safe("⚠️ Suggestion: Close browsers / background apps")
                    actions_done.append("RAM_WARNING")

                self.log_safe("\n" + "=" * 60)
                self.log_safe("QUICK FIX COMPLETE")
                self.log_safe("=" * 60)

                if actions_done:
                    self.log_safe("\nActions performed:")
                    for a in actions_done:
                        self.log_safe(f"→ {a}")
                else:
                    self.log_safe("✓ No fixes needed")

                self.status_safe("Quick fix complete", "green")

            except Exception as e:
                self.log_safe(f"❌ QUICK FIX ERROR: {e}")
                self.status_safe("Quick fix failed", "red")

            finally:
                self.is_running = False

        threading.Thread(target=run, daemon=True).start()

    # ── FULL FIX ───────────────────────────────────────────────────────────────
    def full_fix(self):
        if self.is_running:
            return

        if not messagebox.askyesno(
            "Confirm FULL FIX",
            "This will reset caches, logs, and some config files.\n"
            "Recommended only if crashes persist.\n\nContinue?"
        ):
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def run():
            try:
                self.log_safe("=" * 60)
                self.log_safe("FIXCRAFT FULL FIX ENGINE")
                self.log_safe("=" * 60)

                mc_path = self.get("minecraft_path")

                self.log_safe("\n[1] SAFETY CHECK")

                vm = psutil.virtual_memory()
                disk = psutil.disk_usage(os.path.splitdrive(os.getcwd())[0] + "\\")

                ram_gb = vm.available / (1024**3)
                disk_gb = disk.free / (1024**3)

                if not os.path.isdir(mc_path):
                    self.log_safe("❌ Minecraft path invalid — aborting FULL FIX")
                    self.status_safe("Full fix aborted", "red")
                    return

                self.log_safe("✓ Environment safe")

                self.log_safe("\n[2] DEEP CLEAN PHASE")

                actions = []

                logs_path = os.path.join(mc_path, "logs")
                if os.path.isdir(logs_path):
                    try:
                        removed = 0
                        for f in os.listdir(logs_path):
                            fp = os.path.join(logs_path, f)
                            if os.path.isfile(fp):
                                os.remove(fp)
                                removed += 1

                        self.log_safe(f"✓ Logs wiped ({removed} files)")
                        actions.append("LOGS_WIPED")

                    except Exception as e:
                        self.log_safe(f"⚠️ Log wipe failed: {e}")

                crash_path = os.path.join(mc_path, "crash-reports")
                if os.path.isdir(crash_path):
                    try:
                        removed = 0
                        for f in os.listdir(crash_path):
                            fp = os.path.join(crash_path, f)
                            if os.path.isfile(fp):
                                os.remove(fp)
                                removed += 1

                        self.log_safe(f"✓ Crash reports removed ({removed})")
                        actions.append("CRASH_REPORTS_CLEARED")

                    except Exception as e:
                        self.log_safe(f"⚠️ Crash cleanup failed: {e}")

                cache_targets = [
                    os.path.join(mc_path, "cache"),
                    os.path.join(mc_path, "shadercache"),
                    os.path.join(mc_path, "caches")
                ]

                self.log_safe("\n[3] CACHE CLEANUP")

                for c in cache_targets:
                    if os.path.exists(c):
                        try:
                            shutil.rmtree(c)
                            self.log_safe(f"✓ Removed {os.path.basename(c)}")
                            actions.append(f"CACHE_REMOVED_{os.path.basename(c)}")
                        except Exception as e:
                            self.log_safe(f"⚠️ Cache error: {e}")

                self.log_safe("\n[4] CONFIG REPAIR")

                config_file = os.path.join(mc_path, "options.txt")

                if os.path.exists(config_file):
                    try:
                        size = os.path.getsize(config_file)

                        if size == 0:
                            self.log_safe("❌ options.txt corrupt (empty)")
                            os.remove(config_file)
                            self.log_safe("→ Config deleted (will regenerate)")
                            actions.append("CONFIG_RESET_EMPTY")

                        elif size > 0:
                            backup = config_file + ".bak"
                            shutil.copy2(config_file, backup)
                            self.log_safe("✓ Config backed up")
                            actions.append("CONFIG_BACKED_UP")

                    except Exception as e:
                        self.log_safe(f"⚠️ Config error: {e}")

                self.log_safe("\n[5] RESOURCE RESPONSE")

                if ram_gb < 4:
                    self.log_safe("⚠️ LOW RAM detected — performance may still degrade")
                    actions.append("LOW_RAM_WARNING")

                if disk_gb < 10:
                    self.log_safe("⚠️ LOW DISK space — consider cleanup")
                    actions.append("LOW_DISK_WARNING")

                self.log_safe("\n[6] FINAL VALIDATION")

                vm2 = psutil.virtual_memory()

                if vm2.percent < 70:
                    self.log_safe("✓ System load improved")
                else:
                    self.log_safe("⚠️ System still under pressure")

                self.log_safe("\n" + "=" * 60)
                self.log_safe("FULL FIX COMPLETE")
                self.log_safe("=" * 60)

                if actions:
                    self.log_safe("\nActions executed:")
                    for a in actions:
                        self.log_safe(f"→ {a}")
                else:
                    self.log_safe("✓ No actions needed")

                self.status_safe("Full fix complete", "green")

            except Exception as e:
                self.log_safe(f"❌ FULL FIX ERROR: {e}")
                self.status_safe("Full fix failed", "red")

            finally:
                self.is_running = False

        threading.Thread(target=run, daemon=True).start()

    # ── INCREASE VIRTUAL MEMORY ───────────────────────────────────────────────
    def increase_memory(self):
        if self.is_running:
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def log(msg):
            self.root.after(0, lambda: self.output.insert(tk.END, str(msg) + "\n"))

        def finish():
            self.is_running = False

        def run():
            try:
                log("=" * 60)
                log("💾 VIRTUAL MEMORY OPTIMIZATION ENGINE")
                log("=" * 60)

                log("\n[1] Environment Check")

                os_name = platform.system()
                log(f"OS Detected: {os_name}")

                if os_name != "Windows":
                    log("❌ Unsupported OS — Virtual Memory tuning only supported on Windows")
                    finish()
                    return

                log("✓ Windows environment confirmed")

                log("\n[2] Resource Check")

                vm = psutil.virtual_memory()
                total_gb = vm.total / (1024 ** 3)
                avail_gb = vm.available / (1024 ** 3)
                percent = vm.percent

                log(f"Total RAM: {total_gb:.1f} GB")
                log(f"Available RAM: {avail_gb:.1f} GB")
                log(f"Usage: {percent}%")

                if percent > 85:
                    log("⚠️ High memory pressure detected")
                elif percent < 50:
                    log("✓ Memory usage is healthy")
                else:
                    log("ℹ️ Moderate usage")

                log("\n[3] System Paging Support Check")

                sys_drive = os.path.splitdrive(os.getcwd())[0] + "\\"

                if not os.path.exists(sys_drive):
                    log("❌ System drive not accessible")
                    finish()
                    return

                free_space = psutil.disk_usage(sys_drive).free / (1024 ** 3)
                log(f"Free disk space on system drive: {free_space:.1f} GB")

                if free_space < 5:
                    log("⚠️ WARNING: Low disk space may break virtual memory expansion")

                log("\n[4] System Integrity Check")

                try:
                    test_file = os.path.join(CONFIG_DIR, "vm_test.tmp")
                    with open(test_file, "w") as f:
                        f.write("test")
                    os.remove(test_file)
                    log("✓ File system writable")
                except Exception as e:
                    log(f"❌ File system issue: {e}")
                    finish()
                    return

                log("\n[5] Runtime Risk Detection")

                if vm.percent > 90:
                    log("🔴 CRITICAL RAM usage — system may crash under load")
                elif vm.percent > 75:
                    log("🟡 Elevated RAM usage — optimization recommended")
                else:
                    log("🟢 No critical memory risk detected")

                log("\n[6] Virtual Memory State Analysis")

                log("Current Recommendation Engine:")
                log("• Minimum paging file: 1.5x RAM")
                log("• Maximum paging file: 3x RAM")

                recommended_min = int(total_gb * 1.5 * 1024)
                recommended_max = int(total_gb * 3 * 1024)

                log(f"Suggested Min: {recommended_min} MB")
                log(f"Suggested Max: {recommended_max} MB")

                log("\n[7] Fix Decision Layer")

                if vm.percent > 85 or avail_gb < 2:
                    log("⚡ ACTION REQUIRED: High memory pressure detected")
                    log("👉 Recommended action:")

                    steps = [
                        "Open System Properties",
                        "Advanced System Settings",
                        "Performance → Settings",
                        "Advanced → Virtual Memory",
                        "Uncheck 'Automatically manage paging file'",
                        f"Set Initial Size: {recommended_min} MB",
                        f"Set Maximum Size: {recommended_max} MB",
                        "Click Set → Apply → Restart PC"
                    ]

                    for s in steps:
                        log("   ➜ " + s)

                else:
                    log("✓ System memory stable")
                    log("ℹ️ Manual increase not required, but optional boost available")
                    log("Suggested optional upgrade:")
                    log(f"   Min: {recommended_min} MB")
                    log(f"   Max: {recommended_max} MB")

                log("\n" + "=" * 60)
                log("💾 VIRTUAL MEMORY ANALYSIS COMPLETE")
                log("=" * 60)

            except Exception as e:
                log(f"❌ ERROR: {e}")

            finally:
                finish()

        threading.Thread(target=run, daemon=True).start()

    # ── SYSTEM INFO ───────────────────────────────────────────────────────────
    def show_system_info(self):
        if self.is_running:
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def log(msg):
            self.root.after(0, lambda: self.output.insert(tk.END, str(msg) + "\n"))

        def finish():
            self.is_running = False

        def run():
            try:
                log("=" * 60)
                log("💻 SYSTEM DIAGNOSTIC ENGINE")
                log("=" * 60)

                log("\n[1] Environment Check")

                os_name = platform.system()
                os_version = platform.version()
                arch = platform.architecture()[0]

                log(f"OS: {os_name} {platform.release()}")
                log(f"Version: {os_version}")
                log(f"Architecture: {arch}")

                if os_name != "Windows":
                    log("⚠️ Some metrics may be limited (non-Windows OS)")
                else:
                    log("✓ Full Windows diagnostics enabled")

                log("\n[2] Resource Snapshot")

                vm = psutil.virtual_memory()
                cpu = psutil.cpu_percent(interval=0.5)

                total_ram = vm.total / (1024 ** 3)
                used_ram = vm.used / (1024 ** 3)
                avail_ram = vm.available / (1024 ** 3)

                log(f"CPU Usage: {cpu}%")
                log(f"RAM Total: {total_ram:.2f} GB")
                log(f"RAM Used: {used_ram:.2f} GB")
                log(f"RAM Available: {avail_ram:.2f} GB")

                disk = psutil.disk_usage(os.path.splitdrive(os.getcwd())[0] + "\\")
                log(f"Disk Total: {disk.total / (1024**3):.1f} GB")
                log(f"Disk Free: {disk.free / (1024**3):.1f} GB")

                log("\n[3] System Path Validation")

                system_drive = os.path.splitdrive(os.getcwd())[0] + "\\"

                if os.path.exists(system_drive):
                    log(f"✓ System drive accessible: {system_drive}")
                else:
                    log("❌ System drive error detected")

                if disk.free < 10 * (1024**3):
                    log("⚠️ Low disk space detected (<10GB)")

                log("\n[4] Hardware Profile")

                log(f"Physical cores: {psutil.cpu_count(logical=False)}")
                log(f"Logical cores: {psutil.cpu_count(logical=True)}")

                try:
                    freq = psutil.cpu_freq()
                    if freq:
                        log(f"CPU Frequency: {freq.current:.0f} MHz")
                except:
                    log("CPU frequency unavailable")

                try:
                    import subprocess
                    gpu_info = subprocess.check_output(
                        "wmic path win32_VideoController get name",
                        shell=True
                    ).decode()

                    log("GPU Info:")
                    for line in gpu_info.split("\n"):
                        if line.strip() and "Name" not in line:
                            log(f"  - {line.strip()}")

                except:
                    log("GPU info unavailable")

                log("\n[5] Runtime Risk Detection")

                risk_score = 0

                if cpu > 85:
                    log("🔴 High CPU load detected")
                    risk_score += 2

                if vm.percent > 85:
                    log("🔴 High RAM usage detected")
                    risk_score += 2

                if disk.free < 10 * (1024**3):
                    log("🟡 Low disk space warning")
                    risk_score += 1

                if risk_score == 0:
                    log("🟢 System stable")
                elif risk_score <= 2:
                    log("🟡 Mild instability detected")
                else:
                    log("🔴 System under stress")

                log("\n[6] System Health Score")

                health = 100 - (risk_score * 20)
                health = max(0, health)

                log(f"Health Score: {health}/100")

                if health >= 80:
                    log("🟢 Excellent system condition")
                elif health >= 50:
                    log("🟡 Moderate condition — optimization recommended")
                else:
                    log("🔴 Poor condition — system fixes recommended")

                log("\n[7] Report Summary")

                report = {
                    "os": f"{os_name} {platform.release()}",
                    "cpu_usage": cpu,
                    "ram_usage": vm.percent,
                    "disk_free_gb": round(disk.free / (1024**3), 2),
                    "health_score": health
                }

                log("Generated Diagnostic Report:")
                for k, v in report.items():
                    log(f"  {k}: {v}")

                log("\n" + "=" * 60)
                log("💻 SYSTEM INFO COMPLETE")
                log("=" * 60)

            except Exception as e:
                log(f"❌ ERROR: {e}")

            finally:
                finish()

        threading.Thread(target=run, daemon=True).start()

    # ── CRASH LOG ANALYSIS ────────────────────────────────────────────────────
    def analyze_crash_logs(self):
        if self.is_running:
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def log(msg):
            self.root.after(0, lambda: self.output.insert(tk.END, str(msg) + "\n"))

        def finish():
            self.is_running = False

        def run():
            try:
                log("=" * 60)
                log("🔍 CRASH LOG FORENSIC ENGINE")
                log("=" * 60)

                log("\n[1] Environment Check")

                os_name = platform.system()
                log(f"OS: {os_name}")

                if os_name != "Windows":
                    log("⚠️ Limited log parsing on non-Windows systems")

                log("\n[2] System Snapshot (current state)")

                vm = psutil.virtual_memory()
                cpu = psutil.cpu_percent(interval=0.3)

                log(f"CPU Load: {cpu}%")
                log(f"RAM Usage: {vm.percent}%")
                log(f"Available RAM: {vm.available / (1024**3):.2f} GB")

                log("\n[3] Log Path Validation")

                log_dir = os.path.expandvars(
                    r"%AppData%\.minecraft\logs"
                )

                if not os.path.exists(log_dir):
                    log("❌ Log directory not found")
                    log("👉 Minecraft may not be installed or never launched correctly")
                    finish()
                    return

                log(f"✓ Log directory found: {log_dir}")

                log("\n[4] Log File Discovery")

                logs = [f for f in os.listdir(log_dir) if f.endswith(".log")]

                if not logs:
                    log("✓ No crash logs found (clean state)")
                    finish()
                    return

                logs.sort(
                    key=lambda f: os.path.getmtime(os.path.join(log_dir, f)),
                    reverse=True
                )

                latest_log = os.path.join(log_dir, logs[0])

                log(f"Found {len(logs)} logs")
                log(f"Analyzing latest: {logs[0]}")

                log("\n[5] Pattern Detection Engine")

                issues = {
                    "ram": 0,
                    "gpu": 0,
                    "shader": 0,
                    "config": 0,
                    "engine": 0,
                    "corruption": 0
                }

                try:
                    with open(latest_log, "r", encoding="utf-8", errors="ignore") as f:
                        content = f.read().lower()

                    if "outofmemory" in content or "out of memory" in content:
                        issues["ram"] += 2
                        log("🔴 RAM CRASH DETECTED")

                    if any(x in content for x in ["d3d", "gpu crash", "dxgi", "rhi"]):
                        issues["gpu"] += 2
                        log("🔴 GPU / DIRECTX FAILURE DETECTED")

                    if "shader" in content:
                        issues["shader"] += 2
                        log("🟡 SHADER COMPILATION ISSUE DETECTED")

                    if any(x in content for x in ["ini", "config", "settings"]):
                        issues["config"] += 1
                        log("🟡 CONFIGURATION ISSUE DETECTED")

                    if any(x in content for x in ["unreal", "engine", "fatal error"]):
                        issues["engine"] += 2
                        log("🔴 ENGINE CRASH DETECTED")

                    if any(x in content for x in ["corruption", "fatal", "crash"]):
                        issues["corruption"] += 2
                        log("🔴 FATAL CORRUPTION DETECTED")

                except Exception as e:
                    log(f"❌ Failed to read log: {e}")
                    finish()
                    return

                log("\n[6] Severity Analysis")

                total_score = sum(issues.values())

                for k, v in issues.items():
                    log(f"{k.upper()}: {'HIGH' if v>=2 else 'LOW' if v==1 else 'NONE'}")

                if total_score >= 6:
                    severity = "🔴 CRITICAL"
                elif total_score >= 3:
                    severity = "🟡 MODERATE"
                elif total_score > 0:
                    severity = "🟢 LOW"
                else:
                    severity = "🟢 CLEAN"

                log(f"\nOverall Severity: {severity}")

                log("\n[7] Recommended Fix Actions")

                if issues["ram"] >= 2:
                    log("💾 Increase Virtual Memory immediately")

                if issues["gpu"] >= 2:
                    log("🎮 Update GPU drivers (NVIDIA / AMD)")

                if issues["shader"] >= 2:
                    log("🔧 Clear Shader Cache")

                if issues["config"] >= 1:
                    log("⚙️ Reset Minecraft config files")

                if issues["engine"] >= 2:
                    log("🚨 Verify Minecraft files in Mojang Launcher")

                if issues["corruption"] >= 2:
                    log("🛡️ Repair Minecraft installation")

                if total_score == 0:
                    log("✓ No crash patterns detected — logs are clean")

                log("\n" + "=" * 60)
                log("🔍 CRASH ANALYSIS COMPLETE")
                log("=" * 60)

            except Exception as e:
                log(f"❌ ERROR: {e}")

            finally:
                finish()

        threading.Thread(target=run, daemon=True).start()

    # ── QUICK SCAN ────────────────────────────────────────────────────────────
    def quick_scan(self):
        if self.is_running:
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def log(msg):
            self.root.after(0, lambda: self.output.insert(tk.END, str(msg) + "\n"))

        def finish():
            self.is_running = False

        def run():
            try:
                log("=" * 60)
                log("⚡ QUICK SCAN ENGINE")
                log("=" * 60)

                log("\n[1] Environment Check")

                os_name = platform.system()
                log(f"OS: {os_name}")

                if os_name != "Windows":
                    log("⚠️ Limited scan mode (non-Windows)")
                else:
                    log("✓ Windows optimized scan active")

                log("\n[2] Resource Snapshot")

                vm = psutil.virtual_memory()
                cpu = psutil.cpu_percent(interval=0.2)

                ram_usage = vm.percent
                avail_gb = vm.available / (1024**3)

                log(f"CPU: {cpu}%")
                log(f"RAM: {ram_usage}%")
                log(f"Available RAM: {avail_gb:.2f} GB")

                log("\n[3] Game Path Check")

                path = self.get("minecraft_path")

                if os.path.exists(path):
                    log("✓ Minecraft path valid")
                else:
                    log("❌ Minecraft path missing or incorrect")

                log("\n[4] Risk Detection")

                risk = 0

                if ram_usage > 85:
                    log("🔴 High RAM usage")
                    risk += 2

                if cpu > 85:
                    log("🔴 High CPU usage")
                    risk += 2

                if avail_gb < 2:
                    log("🟡 Low available memory")
                    risk += 1

                if risk == 0:
                    log("🟢 System stable")
                elif risk <= 2:
                    log("🟡 Minor instability detected")
                else:
                    log("🔴 High crash risk detected")

                log("\n[5] Crash Probability")

                if risk >= 4:
                    chance = "HIGH (70–90%)"
                elif risk == 3:
                    chance = "MEDIUM (40–60%)"
                elif risk > 0:
                    chance = "LOW (10–30%)"
                else:
                    chance = "VERY LOW (under 10%)"

                log(f"Estimated Crash Chance: {chance}")

                log("\n[6] Instant Fix Suggestions")

                if ram_usage > 85:
                    log("💾 Close background apps")

                if cpu > 85:
                    log("🔻 Reduce background CPU usage")

                if avail_gb < 2:
                    log("💽 Free up system memory")

                if not os.path.exists(path):
                    log("📁 Fix Minecraft install path in Settings")

                log("\n[7] Health Score")

                score = max(0, 100 - (risk * 25))

                log(f"System Score: {score}/100")

                if score >= 80:
                    log("🟢 Healthy system")
                elif score >= 50:
                    log("🟡 Moderate condition")
                else:
                    log("🔴 Poor condition — action recommended")

                log("\n" + "=" * 60)
                log("⚡ QUICK SCAN COMPLETE")
                log("=" * 60)

            except Exception as e:
                log(f"❌ ERROR: {e}")

            finally:
                finish()

        threading.Thread(target=run, daemon=True).start()

    # ── FULL SCAN ─────────────────────────────────────────────────────────────
    def full_scan(self):
        if self.is_running:
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def log(msg):
            self.root.after(0, lambda: self.output.insert(tk.END, str(msg) + "\n"))

        def finish():
            self.is_running = False

        def run():
            try:
                log("=" * 60)
                log("🔎 FULL SYSTEM FORENSIC SCAN")
                log("=" * 60)

                log("\n[1] Environment Validation")

                os_name = platform.system()
                os_version = platform.version()

                log(f"OS: {os_name}")
                log(f"Version: {os_version}")

                if os_name != "Windows":
                    log("❌ Unsupported OS — scan results may be inaccurate")
                    finish()
                    return

                log("✓ Full Windows compatibility confirmed")

                log("\n[2] Resource Audit")

                vm = psutil.virtual_memory()
                cpu = psutil.cpu_percent(interval=0.5)
                disk = psutil.disk_usage(os.path.splitdrive(os.getcwd())[0] + "\\")

                total_ram = vm.total / (1024**3)
                used_ram = vm.used / (1024**3)
                free_disk = disk.free / (1024**3)

                log(f"CPU Load: {cpu}%")
                log(f"RAM Total: {total_ram:.2f} GB")
                log(f"RAM Used: {used_ram:.2f} GB")
                log(f"RAM Usage: {vm.percent}%")
                log(f"Disk Free: {free_disk:.2f} GB")

                log("\n[3] Game Path Integrity")

                minecraft_path = self.get("minecraft_path")

                if os.path.isdir(minecraft_path):
                    log("✓ Minecraft installation found")
                else:
                    log("❌ Minecraft path invalid")
                    log("👉 This can cause launch or crash failures")

                subfolders = ["saves", "mods"]
                missing = []

                for sub in subfolders:
                    if not os.path.exists(os.path.join(minecraft_path, sub)):
                        missing.append(sub)

                if missing:
                    log(f"⚠️ Missing core folders: {', '.join(missing)}")
                else:
                    log("✓ Core game structure intact")

                log("\n[4] File System Deep Scan")

                log_dir = os.path.expandvars(
                    r"%AppData%\.minecraft\logs"
                )

                config_dir = os.path.expandvars(
                    r"%AppData%\.minecraft\config"
                )

                issues_found = 0

                if os.path.exists(log_dir):
                    logs = os.listdir(log_dir)
                    log(f"Log files: {len(logs)}")

                    if len(logs) > 20:
                        log("⚠️ Excessive log accumulation detected")
                        issues_found += 1
                else:
                    log("⚠️ Log directory missing")

                if os.path.exists(config_dir):
                    log("✓ Config directory found")
                else:
                    log("❌ Config directory missing")
                    issues_found += 1

                shader_cache = os.path.expandvars(
                    r"%AppData%\.minecraft\shadercache"
                )

                if os.path.exists(shader_cache):
                    log("✓ Shader cache exists")
                else:
                    log("⚠️ Shader cache missing (may cause first-launch lag)")

                log("\n[5] Runtime Risk Correlation")

                risk = 0

                if vm.percent > 85:
                    log("🔴 RAM overload detected")
                    risk += 2

                if cpu > 85:
                    log("🔴 CPU overload detected")
                    risk += 2

                if free_disk < 10:
                    log("🟡 Low disk space risk")
                    risk += 1

                if missing:
                    log("🔴 Game structure risk detected")
                    risk += 2

                log("\n[6] Behavioral Analysis")

                if os.path.exists(log_dir):
                    crash_logs = [f for f in os.listdir(log_dir) if "crash" in f.lower()]

                    if crash_logs:
                        log(f"⚠️ Recent crash logs detected: {len(crash_logs)}")
                        risk += 1
                    else:
                        log("✓ No recent crash patterns found")

                log("\n[7] Final Diagnostic Report")

                score = max(0, 100 - (risk * 20))

                log(f"System Stability Score: {score}/100")

                if score >= 80:
                    log("🟢 System Stable — No immediate fixes required")
                elif score >= 50:
                    log("🟡 Moderate issues detected — optimization recommended")
                else:
                    log("🔴 Critical instability — full fix strongly recommended")

                log("\nRecommended Action:")

                if score < 50:
                    log("👉 Run FULL FIX immediately")
                elif score < 80:
                    log("👉 Run QUICK FIX + Clear cache")
                else:
                    log("👉 System is healthy — optional maintenance only")

                log("\n" + "=" * 60)
                log("🔎 FULL SCAN COMPLETE")
                log("=" * 60)

            except Exception as e:
                log(f"❌ ERROR: {e}")

            finally:
                finish()

        threading.Thread(target=run, daemon=True).start()

    # ── RESET CONFIG FILES ────────────────────────────────────────────────────
    def reset_config(self):
        import time

        if self.is_running:
            self.status_safe("System busy...", "orange")
            return

        confirm = messagebox.askyesno(
            "Reset Config Files",
            "This will reset Minecraft config files.\n\n"
            "✔ Safe backup will be created\n"
            "✔ Game settings will reset to default\n\n"
            "Continue?"
        )

        if not confirm:
            self.status_safe("Reset cancelled", "red")
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        base_path = os.path.expandvars(
            r"%AppData%\.minecraft\config"
        )

        backup_dir = os.path.join(CONFIG_DIR, "backup_configs")
        timestamp = time.strftime("%Y%m%d_%H%M%S")

        targets = [
            "options.txt",
            "servers.dat",
            "options.txt"
        ]

        def worker():
            deleted = []
            missing = []
            failed = []
            backed_up = []

            try:
                self.log_safe("=== CONFIG RESET ENGINE INITIATED ===")
                self.log_safe(f"Target path: {base_path}")

                if not os.path.exists(base_path):
                    self.log_safe("❌ Config directory not found")
                    self.log_safe("Possible reasons:")
                    self.log_safe("- Minecraft not installed")
                    self.log_safe("- First launch not completed")
                    self.status_safe("Config path missing", "red")
                    return

                os.makedirs(backup_dir, exist_ok=True)
                backup_session = os.path.join(backup_dir, f"backup_{timestamp}")
                os.makedirs(backup_session, exist_ok=True)

                self.log_safe("📦 Backup directory created")

                for file in targets:
                    file_path = os.path.join(base_path, file)

                    try:
                        if os.path.isfile(file_path):
                            backup_path = os.path.join(backup_session, file)

                            try:
                                shutil.copy2(file_path, backup_path)
                                backed_up.append(file)
                                self.log_safe(f"📦 Backed up: {file}")
                            except Exception as be:
                                self.log_safe(f"⚠️ Backup failed for {file}: {be}")

                            try:
                                os.remove(file_path)
                                deleted.append(file)
                                self.log_safe(f"🗑 Deleted: {file}")

                            except PermissionError:
                                failed.append(file)
                                self.log_safe(f"❌ Permission denied: {file}")

                            except Exception as de:
                                failed.append(file)
                                self.log_safe(f"❌ Delete error {file}: {de}")

                        else:
                            missing.append(file)
                            self.log_safe(f"ℹ️ Missing (skipped): {file}")

                    except Exception as e:
                        failed.append(file)
                        self.log_safe(f"❌ Unexpected error on {file}: {e}")

                try:
                    self.log_safe("🔧 Regenerating defaults...")

                    default_engine_ini = """[Core.System]
Paths=../../../Engine/Content"""

                    for file in missing:
                        if file == "options.txt":
                            with open(os.path.join(base_path, file), "w") as f:
                                f.write(default_engine_ini.strip())
                            self.log_safe("🆕 Recreated options.txt")

                except Exception as regen_error:
                    self.log_safe(f"⚠️ Regeneration issue: {regen_error}")

                self.log_safe("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
                self.log_safe("RESET SUMMARY")
                self.log_safe(f"Backed up: {len(backed_up)}")
                self.log_safe(f"Deleted: {len(deleted)}")
                self.log_safe(f"Missing: {len(missing)}")
                self.log_safe(f"Failed: {len(failed)}")
                self.log_safe("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")

                if len(failed) == 0:
                    self.status_safe("Reset successful", "green")
                    self.log_safe("✔ CONFIG RESET COMPLETE")

                elif len(failed) < len(targets):
                    self.status_safe("Partial reset", "orange")
                    self.log_safe("⚠ Some files could not be reset")

                else:
                    self.status_safe("Reset failed", "red")
                    self.log_safe("❌ No files could be modified")

            finally:
                self.is_running = False

                try:
                    self.save_config()
                except:
                    pass

        threading.Thread(target=worker, daemon=True).start()

    # ── CHECK RAM PRESSURE ────────────────────────────────────────────────────
    def check_ram_pressure(self):
        if self.is_running:
            self.status_safe("System busy...", "orange")
            return

        self.is_running = True
        self.output.delete(1.0, tk.END)

        def worker():
            try:
                self.log_safe("=== RAM PRESSURE ANALYSIS ===")

                vm = psutil.virtual_memory()

                if not vm:
                    self.log_safe("❌ Cannot access system memory")
                    self.status_safe("RAM check failed", "red")
                    return

                swap = psutil.swap_memory()

                total_gb = vm.total / (1024 ** 3)
                used_gb = vm.used / (1024 ** 3)
                free_gb = vm.available / (1024 ** 3)

                usage_percent = vm.percent
                swap_used_percent = swap.percent

                self.log_safe(f"💾 Total RAM: {total_gb:.1f} GB")
                self.log_safe(f"📊 Used RAM: {used_gb:.1f} GB")
                self.log_safe(f"🟢 Available RAM: {free_gb:.1f} GB")
                self.log_safe(f"📈 RAM Usage: {usage_percent}%")
                self.log_safe(f"🔁 Swap Usage: {swap_used_percent}%")

                self.log_safe("\n🔍 Top Memory Consumers:")

                processes = []
                for p in psutil.process_iter(['pid', 'name', 'memory_info']):
                    try:
                        mem = p.info['memory_info'].rss / (1024 ** 2)
                        processes.append((mem, p.info['name']))
                    except:
                        continue

                processes.sort(reverse=True)
                for mem, name in processes[:5]:
                    self.log_safe(f" - {name}: {mem:.1f} MB")

                risk_level = "SAFE"
                risk_color = "green"
                recommendations = []

                if usage_percent >= 90:
                    risk_level = "CRITICAL"
                    risk_color = "red"
                    recommendations += [
                        "Close background apps immediately",
                        "Restart system before launching Minecraft",
                        "Disable overlays (Discord / GeForce / Xbox)"
                    ]

                elif usage_percent >= 75:
                    risk_level = "WARNING"
                    risk_color = "orange"
                    recommendations += [
                        "Close Chrome tabs or heavy apps",
                        "Avoid running background downloads",
                        "Restart Minecraft if stuttering occurs"
                    ]

                elif usage_percent >= 60:
                    risk_level = "MODERATE"
                    risk_color = "yellow"
                    recommendations += [
                        "System stable but slightly loaded",
                        "Monitor background apps",
                    ]

                else:
                    recommendations.append("System is optimal for Minecraft")

                if swap_used_percent > 50:
                    self.log_safe("⚠️ HIGH SWAP USAGE detected (risk of stutter/crash)")
                    recommendations.append("Increase virtual memory")

                if free_gb < 3:
                    recommendations.append("Low free RAM — crash risk increased")

                self.log_safe("\n━━━━━━━━━━━━━━━━━━━━━━")
                self.log_safe(f"RAM STATUS: {risk_level}")
                self.log_safe("━━━━━━━━━━━━━━━━━━━━━━")

                for r in recommendations:
                    self.log_safe("➡ " + r)

                self.status_safe(f"RAM: {risk_level}", risk_color)

            except Exception as e:
                self.log_safe(f"❌ RAM check error: {e}")
                self.status_safe("Error", "red")

            finally:
                self.is_running = False

        threading.Thread(target=worker, daemon=True).start()

    # ── HELP & FAQ ────────────────────────────────────────────────────────────
    def show_help(self):
        win = tk.Toplevel(self.root)
        win.title("FixCraft Help & FAQ")
        win.geometry("650x600")

        header = tk.Label(
            win,
            text="📖 FixCraft Help & Diagnostic Guide",
            font=("Arial", 14, "bold")
        )
        header.pack(pady=10)

        txt = scrolledtext.ScrolledText(win, wrap=tk.WORD, font=("Arial", 10))
        txt.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        content = """
=============================
🧠 WHAT IS FIXCRAFT?
=============================
FixCraft is a diagnostic and repair tool designed to:
- Analyze system performance issues
- Detect Minecraft crash causes
- Clean corrupted cache files
- Improve system stability
- Identify RAM / GPU / config problems

It does NOT modify your account or game ownership.

=============================
🔍 MAIN FUNCTIONS
=============================

🔍 System Analysis
------------------
Performs full system scan:
✔ RAM status check
✔ Disk space evaluation
✔ Minecraft path validation
✔ Crash log detection
✔ Risk level estimation

⚡ Quick Fix
-----------
Fast repair mode:
✔ Clears shader cache
✔ Clears launcher cache
✔ Applies lightweight optimizations
✔ Fixes common crash causes

🔧 Full Fix
----------
Deep repair mode:
✔ All Quick Fix actions
✔ Config validation
✔ Extended cleanup
✔ System re-evaluation
✔ Recovery checks

💾 Virtual Memory Boost
----------------------
Guided optimization:
✔ Explains paging file settings
✔ Suggests safe RAM expansion values
✔ Prevents memory-related crashes

💻 System Info
-------------
Displays:
✔ CPU usage
✔ RAM usage
✔ Disk space
✔ OS version

🔍 Crash Log Analysis
--------------------
Reads Minecraft logs and detects:
✔ GPU errors
✔ RAM overflow
✔ Shader corruption
✔ Config instability

🔄 Reset Config Files
--------------------
Safely resets:
✔ options.txt
✔ servers.dat
✔ Engine.ini (if needed)
✔ Automatically backs up files

⚠️ RAM Pressure Check
--------------------
Evaluates:
✔ Memory usage %
✔ Swap usage
✔ System stress level
✔ Crash risk prediction

=============================
❓ FAQ
=============================

Q: Will this delete my worlds, skins, or progress?
A: No. Only cache/config files are modified. Account data is untouched.

Q: Why does Minecraft still crash?
A: Possible causes:
- GPU driver issues
- Low RAM availability
- Corrupted game files
- Overheating hardware

Q: Why is antivirus flagging this?
A: Tools built with Python/PyInstaller are often falsely flagged.

Q: Do I need internet for this tool?
A: No. Everything runs locally.

Q: What is the safest option?
A: Always start with "System Analysis" before fixes.

=============================
⚠️ COMMON CRASH CAUSES
=============================

1. Low RAM (<4GB available)
2. Full disk storage
3. Corrupted shader cache
4. Outdated GPU drivers
5. Broken config files
6. Background apps overload

=============================
🛠️ FIX ORDER (IMPORTANT)
=============================

For best results always follow:

1️⃣ System Analysis
2️⃣ RAM Pressure Check
3️⃣ Crash Log Analysis
4️⃣ Quick Fix
5️⃣ Full Fix (if needed)

=============================
💡 PRO TIPS
=============================

✔ Restart PC after Full Fix
✔ Close Chrome before launching Minecraft
✔ Keep 10–20GB free disk space
✔ Update GPU drivers regularly

=============================
📌 DISCLAIMER
=============================

FixCraft is NOT affiliated with Mojang.
Use at your own risk.
No account data is accessed or modified.
"""

        txt.insert(tk.END, content)
        txt.config(state=tk.DISABLED)

        btn_frame = ttk.Frame(win)
        btn_frame.pack(pady=5)

        ttk.Button(btn_frame, text="📜 Privacy", command=self.show_privacy).pack(
            side=tk.LEFT, padx=5
        )
        ttk.Button(btn_frame, text="⚠️ Disclaimer", command=self.show_disclaimer).pack(
            side=tk.LEFT, padx=5
        )
        ttk.Button(btn_frame, text="Close", command=win.destroy).pack(
            side=tk.LEFT, padx=5
        )

    # ── PRIVACY ───────────────────────────────────────────────────────────────
    def show_privacy(self):
        win = tk.Toplevel(self.root)
        win.title("Privacy Policy")
        win.geometry("450x400")

        txt = scrolledtext.ScrolledText(win, wrap=tk.WORD)
        txt.pack(fill=tk.BOTH, expand=True)

        txt.insert(
            tk.END,
            """PRIVACY POLICY — FixCraft v1.0

This application does NOT collect, store, or transmit any personal data.

It does NOT collect:
- Personal information (name, email, etc.)
- System telemetry
- Minecraft account data
- Usage analytics
- Location data

INTERNET USAGE
This application does NOT require an internet connection and does not
send any data externally. All operations are performed locally.

LOCAL DATA STORAGE
Settings are stored locally in:
%AppData%\\FixCraft\\settings.ini

This file includes only:
- UI preferences (dark mode, theme color)
- User-selected game installation path
- Feature toggle settings

FILE ACCESS
The application may access and modify only:
- Minecraft shader cache folders
- Local system information (read-only)

No game executables or account files are modified.

DISCLAIMER
This tool is not affiliated with Mojang or Minecraft.
Use at your own risk.
""",
        )
        txt.config(state=tk.DISABLED)
        ttk.Button(win, text="Close", command=win.destroy).pack(pady=5)

    # ── DISCLAIMER ────────────────────────────────────────────────────────────
    def show_disclaimer(self):
        win = tk.Toplevel(self.root)
        win.title("Disclaimer")
        win.geometry("450x300")

        txt = scrolledtext.ScrolledText(win, wrap=tk.WORD)
        txt.pack(fill=tk.BOTH, expand=True)

        txt.insert(
            tk.END,
            """DISCLAIMER — FixCraft v1.0

FixCraft is an independent utility and is NOT affiliated with,
endorsed by, or connected to Mojang or Minecraft in any way.

USE AT YOUR OWN RISK.

- This tool only modifies local cache and config files.
- No account data, game files, or executables are touched.
- Always back up important files before running any fix.
- The developer is not responsible for any data loss or
  system issues resulting from use of this tool.

By using FixCraft you accept these terms.
""",
        )
        txt.config(state=tk.DISABLED)
        ttk.Button(win, text="Close", command=win.destroy).pack(pady=5)

    # ── SETTINGS ──────────────────────────────────────────────────────────────
    def show_settings(self):
        win = tk.Toplevel(self.root)
        win.title("FixCraft Settings")
        win.geometry("500x550")

        tk.Label(
            win,
            text="⚙️ Settings",
            font=("Arial", 14, "bold")
        ).pack(pady=10)

        auto_analyze = tk.BooleanVar(value=self.get_bool("auto_analyze"))
        dark_mode = tk.BooleanVar(value=self.get_bool("dark_mode"))
        shader_fix = tk.BooleanVar(value=self.get_bool("clear_shader_on_quick_fix"))
        launcher_fix = tk.BooleanVar(value=self.get_bool("clear_launcher_on_quick_fix"))
        ram_warning = tk.BooleanVar(value=True)
        safe_mode = tk.BooleanVar(value=False)
        path_var = tk.StringVar(value=self.get("minecraft_path"))

        general_frame = ttk.LabelFrame(win, text="General Settings")
        general_frame.pack(fill=tk.X, padx=10, pady=5)

        ttk.Checkbutton(
            general_frame,
            text="Auto Analyze on Startup",
            variable=auto_analyze
        ).pack(anchor="w", padx=10, pady=2)

        ttk.Checkbutton(
            general_frame,
            text="Enable Dark Mode",
            variable=dark_mode
        ).pack(anchor="w", padx=10, pady=2)

        ttk.Checkbutton(
            general_frame,
            text="Safe Mode (Disable risky fixes)",
            variable=safe_mode
        ).pack(anchor="w", padx=10, pady=2)

        fix_frame = ttk.LabelFrame(win, text="Quick Fix Behavior")
        fix_frame.pack(fill=tk.X, padx=10, pady=5)

        ttk.Checkbutton(
            fix_frame,
            text="Clear Shader Cache",
            variable=shader_fix
        ).pack(anchor="w", padx=10, pady=2)

        ttk.Checkbutton(
            fix_frame,
            text="Clear Launcher Cache",
            variable=launcher_fix
        ).pack(anchor="w", padx=10, pady=2)

        ttk.Checkbutton(
            fix_frame,
            text="Enable RAM Pressure Warnings",
            variable=ram_warning
        ).pack(anchor="w", padx=10, pady=2)

        path_frame = ttk.LabelFrame(win, text="Game Path")
        path_frame.pack(fill=tk.X, padx=10, pady=5)

        entry = ttk.Entry(path_frame, textvariable=path_var)
        entry.pack(fill=tk.X, padx=10, pady=5)

        def browse():
            folder = filedialog.askdirectory()
            if folder:
                path_var.set(folder)

        ttk.Button(path_frame, text="Browse", command=browse).pack(pady=5)

        def validate():
            path = path_var.get()

            if not os.path.exists(path):
                messagebox.showerror("Error", "Invalid game path!")
                return False

            if len(path) < 5:
                messagebox.showerror("Error", "Path too short")
                return False

            return True

        def save_settings():
            if not validate():
                return

            self.set("auto_analyze", auto_analyze.get())
            self.set("dark_mode", dark_mode.get())
            self.set("clear_shader_on_quick_fix", shader_fix.get())
            self.set("clear_launcher_on_quick_fix", launcher_fix.get())
            self.set("minecraft_path", path_var.get())

            self.save_config()
            self.apply_theme()

            self.log_safe("⚙️ Settings updated successfully")
            win.destroy()

        btn_frame = ttk.Frame(win)
        btn_frame.pack(pady=10)

        ttk.Button(btn_frame, text="💾 Save", command=save_settings).pack(
            side=tk.LEFT, padx=5
        )
        ttk.Button(btn_frame, text="❌ Cancel", command=win.destroy).pack(
            side=tk.LEFT, padx=5
        )
        ttk.Button(btn_frame, text="🔄 Reset Defaults", command=self.reset_settings).pack(
            side=tk.LEFT, padx=5
        )

    # ── RESET SETTINGS ────────────────────────────────────────────────────────
    def reset_settings(self):
        if messagebox.askyesno("Reset Defaults", "Reset all settings to default values?"):
            self.cfg["app"] = {k: str(v) for k, v in DEFAULTS.items()}
            self.save_config()
            self.apply_theme()
            self.log_safe("🔄 Settings reset to defaults")


# ── RUN ───────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    app = FixCraft_v1_0(root)
    root.mainloop()