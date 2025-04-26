#!/home/hododaq/anaconda3/bin/python

import subprocess
import ttkbootstrap as ttk
from ttkbootstrap.constants import *
import os
import threading
import time
import sys
from datetime import datetime
import socket
from PIL import Image, ImageTk  # Only needed for PNG icons


class ConsoleRedirector:
    """Redirects stdout and stderr to the Tkinter Text widget."""
    def __init__(self, text_widget):
        self.text_widget = text_widget

        # Configure text tag colors
        self.text_widget.tag_config("INFO", foreground="white")
        self.text_widget.tag_config("WARNING", foreground="orange")
        self.text_widget.tag_config("ERROR", foreground="red")
        self.text_widget.tag_config("SUCCESS", foreground="lightgreen")
    def write(self, message, level="INFO"):
        timestamp = datetime.now().strftime("[%Y-%m-%d %H:%M:%S] ")
        self.text_widget.configure(state="normal")  # Enable editing
        self.text_widget.insert("end", timestamp, "INFO")
        self.text_widget.insert("end", message+ "\n", level)     # Insert text at the end
        self.text_widget.configure(state="disabled")  # Disable editing
        self.text_widget.see("end")  # Auto-scroll to the latest output

    def flush(self):
        pass  # Needed for compatibility with sys.stdout/sys.stderr


class DAQControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Hodoscope DAQ")

        icon_path = "/home/hododaq/hodo_daq/icons/daq_icon.png"
        icon = Image.open(icon_path)  # Open PNG icon
        icon = ImageTk.PhotoImage(icon)
        self.root.iconphoto(False, icon)

        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()
        self.root.geometry(f"{int(screen_width*0.7)}x{int(screen_height*0.75)}+0+0")

        self.font_size = max(12, int(screen_height / 120))  # Scale with screen height, min size 12
        self.button_font = ("clean", self.font_size)
        self.label_font = ("clean", self.font_size)
        self.text_font = ("clean", int(self.font_size))
        self.title_font = ("clean", int(self.font_size*1.5))

        # Configure ttk Style
        style = ttk.Style()
        style.configure("TButton", font=self.button_font)
        style.configure("TLabel", font=self.label_font)
        style.configure("CustomToggle.TCheckbutton", bootstyle="success-round-toggle", font=self.label_font)

        self.root.grid_rowconfigure((0,1,2,3,4,6), weight=1)
        self.root.grid_rowconfigure(5, weight=20)
        self.root.grid_columnconfigure((0, 1, 2, 3, 4, 5), weight=10)
        # self.root.grid_columnconfigure(5, weight=40)

        # Console output to text box
        self.console_output = ttk.Text(root, height=10, width=300, wrap="word", state="disabled", font=self.text_font)
        # self.console_output = ScrolledText(root, height=10, width=160, wrap="word", state="disabled", font=self.text_font) #
        self.console_output.grid(row=6, column=0, columnspan=6, sticky="s", padx=20, pady=20)

        # Redirect stdout & stderr
        self.console = ConsoleRedirector(self.console_output)
        sys.stdout = self.console
        sys.stderr = lambda msg: self.console.write(msg, "ERROR")

        # Scrollbar
        self.scrollbar = ttk.Scrollbar(root, command=self.console_output.yview, bootstyle="info-round", orient="vertical")
        self.scrollbar.grid(row=6, column=6, sticky="nse")
        self.console_output.config(yscrollcommand=self.scrollbar.set)

        # Read configuration
        self.config_file = "./config/daq_config.conf"  # Change this to your actual config file path
        self.config = self.read_config()
        self.run_number = self.config.get("run_number", "Unknown")  # Get run_number or default to "Unknown"

        # CUSP run number file
        self.run_file = "CUSP/Number.txt"  # Change this to your actual run number file
        self.cusp_number = self.read_cusp_run_number().get("cusp_run", 0)

        # Title Label
        ttk.Label(root, text="Hodoscope Run Control", font=self.title_font).grid(row=0, column=0, columnspan=6, sticky="", padx=20, pady=20)

        # Run Number Display
        self.run_number_label = ttk.Label(root, text=f"Run Number: {self.run_number}", font=self.label_font, bootstyle="info")
        self.run_number_label.grid(row=1, column=1, sticky="", padx=20, pady=20)

        # CUSP Run Number Display
        self.cusp_number_label = ttk.Label(root, text=f"CUSP Run: {self.cusp_number}", font=self.label_font, bootstyle="info")
        self.cusp_number_label.grid(row=2, column=1, sticky="n", padx=20, pady=20)

        # Auto run checkbox
        self.auto_run_var = ttk.BooleanVar()
        self.auto_run_checkbox = ttk.Checkbutton(root, text=" ", variable=self.auto_run_var, 
                                                 command=self.toggle_auto_run, bootstyle="success-round-toggle", 
                                                 state="disabled")
        # self.auto_run_checkbox.configure(style="CustomToggle.TCheckbutton", bootstyle="success-round-toggle")
        self.auto_run_checkbox.grid(row=2, column=0, sticky="ne", padx=30, pady=20)
        self.auto_run_label = ttk.Label(root, text="Auto Run", font=self.label_font, bootstyle="secondary")
        self.auto_run_label.grid(row=2, column=0, sticky="n", padx=50, pady=20)

        # Boolean to track if a run is active
        self.run_active = False

        # Run duration input
        self.run_duration_var = ttk.IntVar(value=80)  # Default: 80
        ttk.Label(root, text="Run Duration (s):", font=self.label_font).grid(row=2, column=2, sticky="ne", padx=20, pady=20)
        self.run_duration_entry = ttk.Entry(root, textvariable=self.run_duration_var, font=self.label_font, width=8)
        self.run_duration_entry.grid(row=2, column=3, sticky="nw", padx=20, pady=20)

        # Run progress bar
        self.run_progress_bar = ttk.Floodgauge(root, bootstyle="dark", maximum=self.run_duration_var.get(), value=0, text="Not Running", length = int(screen_width*0.75/4), mode="determinate", font=self.text_font)
        self.run_progress_bar.grid(row=2, column=4, columnspan=2, sticky="nw", padx=20, pady=10)


        # Buttons
        self.button_width = 15
        self.start_daq_button = ttk.Button(root, text="Start DAQ", command=self.start_daq, 
                                           width=self.button_width, bootstyle="success")
        self.start_daq_button.grid(row=1, column=0, sticky="", padx=20, pady=20)

        self.stop_daq_button = ttk.Button(root, text="! Stop DAQ", command=self.stop_daq, 
                                          width=self.button_width, bootstyle="danger", state="disabled")
        self.stop_daq_button.grid(row=1, column=5, sticky="e", padx=20, pady=20)

        self.start_button = ttk.Button(root, text="Start Run", command=self.start_run, 
                                       width=self.button_width, bootstyle="success", state="disabled")
        self.start_button.grid(row=3, column=0, sticky="n", padx=20, pady=20)

        self.stop_button = ttk.Button(root, text="Stop Run", command=self.stop_run, 
                                      width=self.button_width, bootstyle="danger", state="disabled")
        self.stop_button.grid(row=3, column=1, sticky="n", padx=20, pady=20)

        self.pause_button = ttk.Button(root, text="Pause", command=self.pause_daq, 
                                       width=self.button_width, bootstyle="warning", state="disabled")
        
        self.pause_button.grid(row=3, column=2, sticky="n", padx=20, pady=20)

        self.resume_button = ttk.Button(root, text="Resume", command=self.resume_daq, 
                                        width=self.button_width, bootstyle="success", state="disabled")
        
        self.resume_button.grid(row=3, column=3, columnspan = 1, sticky="n", padx=20, pady=20)

        # Live analysis checkbox
        self.live_analysis_var = ttk.BooleanVar()
        self.live_analysis_checkbox = ttk.Checkbutton(root, text=" ", variable=self.live_analysis_var, 
                                                      command=self.toggle_live_analysis, bootstyle="success-round-toggle", state="disabled")
        self.live_analysis_checkbox.grid(row=4, column=0, sticky="ne", padx=30, pady=20)
        self.live_analysis_label = ttk.Label(root, text="Live Analysis", font=self.label_font, bootstyle="secondary")
        self.live_analysis_label.grid(row=4, column=0, sticky="n", padx=50, pady=20)


        self.daq_process = None
        self.run_running = False


    def send_command(self, command):
        """Send a TCP command to the DAQ controller."""

        # TCP connection settings
        HOST = "127.0.0.1"  # Change to your DAQ controller's IP
        PORT = 12345        # Change to your chosen port

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                s.sendall(command.encode())
                self.console.write(f"Sent: {command}", "INFO")
        except ConnectionRefusedError:
            self.console.write("Error: Connection Refused, you need to run hodo_daq first!", "ERROR")
        except Exception as e:
            self.console.write(f"Error: {e}", "ERROR")

    def start_run(self):
        self.send_command("start")
        self.stop_button.config(state="normal")
        self.start_button.config(state="disabled")
        self.run_running = True
        self.update_floodgauge()
        self.update_run_number()

    def stop_run(self):
        self.send_command("stop")
        self.start_button.config(state="normal")
        self.stop_button.config(state="disabled")
        self.run_running = False
        self.run_progress_bar["value"] = 0
        self.run_progress_bar["text"] = "Not Running"

    def pause_daq(self):
        self.send_command("pause")
        self.pause_button.config(state="disabled")
        self.resume_button.config(state="normal")

    def resume_daq(self):
        self.send_command("resume")
        self.pause_button.config(state="normal")
        self.resume_button.config(state="disabled")

    def stop_daq(self):
        if self.run_running:
            self.send_command("stop")
            self.run_running = False
            self.run_progress_bar["value"] = 0
            self.run_progress_bar["text"] = "Not Running"

        if self.daq_process:
            self.send_command("!")
            self.start_button.config(state="disabled")
            self.stop_button.config(state="disabled")
            self.pause_button.config(state="disabled")
            self.resume_button.config(state="disabled")
            self.auto_run_checkbox.config(state="disabled")
            self.auto_run_label.config(bootstyle="success")
            self.auto_run_enabled = False
            self.daq_process.terminate()
            self.daq_process = None
    
    def start_daq(self):
        if self.daq_process is None or self.daq_process.poll() is not None:
            geometry = "120x24+600+1200"
            self.daq_process = subprocess.Popen(["gnome-terminal",  f"--geometry={geometry}", 
                                                 "--title=DAQ Terminal", "--", "bash", "-c", 
                                                 "cd daq_control/build; ./hodo_daq; exec bash"])
        self.start_button.config(state="normal")
        self.stop_button.config(state="normal")
        self.pause_button.config(state="normal")
        self.resume_button.config(state="normal")
        self.auto_run_checkbox.config(state="normal")
        self.auto_run_label.config(bootstyle="success")
        self.auto_run_enabled = True
        self.live_analysis_checkbox.config(state="normal")
        self.live_analysis_label.config(bootstyle="success")
        self.stop_daq_button.config(state="normal")

    def read_config(self):
        """Reads the daq_config file and returns a dictionary with key-value pairs."""
        config = {}
        try:
            with open(self.config_file, "r") as f:
                for line in f:
                    if "=" in line:
                        key, value = line.strip().split("=", 1)
                        config[key] = value
        except FileNotFoundError:
            self.console.write(f"Warning: {self.config_file} not found.", "WARNING")
        return config

    def update_run_number(self):
        """Updates the displayed run number by rereading the config file."""
        new_config = self.read_config()
        new_run_number = new_config.get("run_number", "Unknown")

        if new_run_number != self.run_number:
            self.run_number = new_run_number
            self.run_number_label.config(text=f"Run Number: {self.run_number}")

        # Schedule the next update in 5 seconds
        self.root.after(5000, self.update_run_number)

    def read_cusp_run_number(self):
        """Reads the CUSP run number file."""
        cusp = {}
        if os.path.exists(self.run_file):
            with open(self.run_file, "r") as f:
                cusp["cusp_run"] = f.read()
                self.console.write(f"CUSP number: {cusp["cusp_run"]}", "SUCCESS")
                self.cusp_number = cusp["cusp_run"]
        else:
            self.console.write("CUSP number file could not be found!", "ERROR")
            cusp["cusp_run"] = 0
        return cusp
        
    def update_floodgauge(self, elapsed_time=0):
        """Updates the Floodgauge every second while data is being taken."""
        self.run_progress_bar["value"] = 0  # Reset gauge at start
        self.run_progress_bar["maximum"] = self.run_duration_var.get()  # Set max duration
        self.run_progress_bar.configure(text="Running...", bootstyle="success")
        
        self.run_progress_bar["value"] = elapsed_time
        # self.run_progress_bar["text"] = f"{elapsed_time} / {self.run_duration_var.get()} sec"

        if elapsed_time < self.run_duration_var.get() and self.run_running:
            self.root.after(1000, self.update_floodgauge, elapsed_time + 1)  # Call itself after 1 sec
        if not self.run_running:
            self.run_progress_bar["value"] = 0
            self.run_progress_bar.configure(text="Not Running", bootstyle="dark")


    def toggle_auto_run(self):
        """Enables or disables auto-run."""
        if self.auto_run_var.get():
            self.auto_run_enabled = True
            threading.Thread(target=self.auto_run_monitor, daemon=True).start()
            self.start_button.config(state="disabled")
            self.stop_button.config(state="disabled")
            self.pause_button.config(state="disabled")
            self.resume_button.config(state="disabled")
        else:
            self.auto_run_enabled = False
            self.start_button.config(state="normal")
            self.stop_button.config(state="normal")
            self.pause_button.config(state="normal")
            self.resume_button.config(state="normal")

    def auto_run_monitor(self):
        """Constantly checks the CUSP Run Number file for changes."""
        while self.auto_run_enabled:
            new_cusp_run = self.read_cusp_run_number().get("cusp_run")
            if new_cusp_run and new_cusp_run != self.current_cusp_run:

                if self.run_active:  
                    self.console.write(f"Stopping previous run before starting new run {new_cusp_run}.", "WARNING")
                    self.stop_run()
                    time.sleep(5)  # Ensure the stop is fully processed

                self.console.write(f"Starting new CUSP run: {new_cusp_run}", "SUCCESS")
                self.start_run()
                self.current_cusp_run = new_cusp_run
                self.run_active = True

                # Get run duration (0 means no auto-stop)
                run_duration = self.run_duration_var.get()
                if run_duration > 0:
                    threading.Thread(target=self.auto_stop_run, args=(run_duration,), daemon=True).start()
            
            time.sleep(1)  # Check every second

    def auto_stop_run(self, duration):
        """Automatically stops the run after a set duration."""
        self.console.write(f"Run will auto-stop in {duration} seconds.", "INFO")
        time.sleep(duration)
        if self.run_active:  
            self.console.write("Auto-stopping the run due to time limit.", "WARNING")
            self.stop_run()
            self.run_active = False

    def toggle_live_analysis(self):
        """Enables or disables live analysis."""
        if self.live_analysis_var.get():
            self.live_analysis_enabled = True
            threading.Thread(target=self.live_analysis_monitor, daemon=True).start()
        else:
            self.live_analysis_enabled = False

    def live_analysis_monitor(self):
        """Starts analysis and waits for incoming data."""
        subprocess.Popen(["xterm", "-e", "./data_analysis/build/hodo_analysis", str(self.run_number)])


# Run the application
if __name__ == "__main__":
    root = ttk.Window(themename="superhero")  # Dark mode theme
    app = DAQControllerApp(root)
    root.mainloop()