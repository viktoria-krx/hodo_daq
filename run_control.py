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
from tkinter import messagebox
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import zmq


plt.style.use("ggplot")

plt.set_cmap("viridis")
plt.rcParams["grid.linestyle"] = "--"
plt.rcParams["grid.color"] = "lightgray"
plt.rcParams["grid.linewidth"] = 0.8
plt.rcParams["axes.axisbelow"] = True
plt.rcParams["font.family"] = "sans-serif"
plt.rcParams.update({'font.size': 8})
plt.rcParams['pdf.fonttype'] = 'truetype'
plt.rcParams["axes.prop_cycle"] = plt.cycler(color=["#316D97", "#DD5043", "#65236E", "#6D904F", "#FFC636", "#8b8b8b",  # Original colors
                                                    "#4a979e", "#e78748", "#4F5D75", "#556B2F", "#B0A8B9"])
plt.rcParams["grid.color"] = "lightgray"
plt.rcParams["grid.linewidth"] = 0.8
plt.rcParams["xtick.direction"] = "in"
plt.rcParams["ytick.direction"] = "in"
plt.rcParams["xtick.top"] = True
plt.rcParams["xtick.bottom"] = True
plt.rcParams["ytick.left"] = True
plt.rcParams["ytick.right"] = True
plt.rcParams["xtick.color"] = "lightgray"
plt.rcParams["ytick.color"] = "lightgray"
plt.rcParams["text.color"] = "lightgray"
plt.rcParams["axes.labelcolor"] = "lightgray"
plt.rcParams["axes.titlecolor"] = "lightgray"
plt.rcParams["figure.facecolor"] = "#ffffff"
plt.rcParams['pdf.fonttype'] = 'truetype'
plt.rcParams["figure.dpi"] = 150
plt.rcParams["savefig.facecolor"] = (1.0, 1.0, 1.0, 0.0)


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


def on_closing():
    if messagebox.askokcancel("Quit", "Do you really want to quit?"):

        # TCP connection settings
        HOST = "127.0.0.1"  # Change to your DAQ controller's IP
        PORT = 12345        # Change to your chosen port
        command = "!"
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                s.sendall(command.encode())
                print(f"Sent: {command}", "INFO")
        except ConnectionRefusedError:
            print("Error: Connection Refused, you need to run hodo_daq first!", "ERROR")
        except Exception as e:
            print(f"Error: {e}", "ERROR")

        root.destroy()

class DAQControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Hodoscope DAQ")

        icon_path = "/home/hododaq/DAQ/icons/daq_icon.png"
        icon = Image.open(icon_path)  # Open PNG icon
        icon = ImageTk.PhotoImage(icon)
        self.root.iconphoto(False, icon)

        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()
        self.root.geometry(f"{int(screen_width*0.55)}x{int(screen_height*0.9)}+0+0")

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
        #self.root.grid_rowconfigure(2, weight=2)
        self.root.grid_rowconfigure(5, weight=15)
        self.root.grid_columnconfigure((0, 1, 2, 3, 4, 5), weight=10)
        # self.root.grid_columnconfigure(5, weight=40)

        # Console output to text box
        self.console_output = ttk.Text(root, height=10, width=300, wrap="word", state="disabled", font=self.text_font)
        # self.console_output = ScrolledText(root, height=10, width=160, wrap="word", state="disabled", font=self.text_font) #
        self.console_output.grid(row=6, column=0, columnspan=6, sticky="s", padx=20, pady=20)

        # Redirect stdout & stderr
        self.console = ConsoleRedirector(self.console_output)
        sys.stdout = self.console
        sys.stderr = self.console 

        # Scrollbar
        self.scrollbar = ttk.Scrollbar(root, command=self.console_output.yview, bootstyle="info-round", orient="vertical")
        self.scrollbar.grid(row=6, column=6, sticky="nse")
        self.console_output.config(yscrollcommand=self.scrollbar.set)

        # Read configuration
        self.config_file = "./config/daq_config.conf"  
        self.config = self.read_config()
        self.run_number = self.config.get("run_number", "Unknown")  # Get run_number or default to "Unknown"

        # CUSP run number file
        self.run_file = "CUSP/Number.txt"  # Change this to your actual run number file
        self.cusp_number = self.read_cusp_run_number().get("cusp_run", 0)

        # Title Label
        ttk.Label(root, text="Hodoscope Run Control", font=self.title_font).grid(row=0, column=0, columnspan=6, sticky="", padx=20, pady=10)

        # Run Number Display
        self.run_number_label = ttk.Label(root, text=f"Run Number: {self.run_number}", font=self.label_font, bootstyle="info")
        self.run_number_label.grid(row=1, column=1, sticky="", padx=20, pady=10)

        # CUSP Run Number Display
        self.cusp_number_label = ttk.Label(root, text=f"CUSP Run: {self.cusp_number}", font=self.label_font, bootstyle="info")
        self.cusp_number_label.grid(row=2, column=1, sticky="n", padx=20, pady=10)

        # Auto run checkbox
        self.auto_run_var = ttk.BooleanVar()
        self.auto_run_checkbox = ttk.Checkbutton(root, text=" ", variable=self.auto_run_var, 
                                                 command=self.toggle_auto_run, bootstyle="success-round-toggle", 
                                                 state="disabled")
        # self.auto_run_checkbox.configure(style="CustomToggle.TCheckbutton", bootstyle="success-round-toggle")
        self.auto_run_checkbox.grid(row=2, column=0, sticky="ne", padx=30, pady=10)
        self.auto_run_label = ttk.Label(root, text="Auto Run", font=self.label_font, bootstyle="secondary")
        self.auto_run_label.grid(row=2, column=0, sticky="nw", padx=50, pady=10)

        # Boolean to track if a run is active
        self.run_active = False

        # Run duration input
        self.run_duration_var = ttk.IntVar(value=80)  # Default: 80
        ttk.Label(root, text="Run Duration (s):", font=self.label_font).grid(row=2, column=2, sticky="ne", padx=20, pady=10)
        self.run_duration_entry = ttk.Entry(root, textvariable=self.run_duration_var, font=self.label_font, width=8)
        self.run_duration_entry.grid(row=2, column=3, sticky="nw", padx=20, pady=10)

        # Run progress bar
        self.run_progress_bar = ttk.Floodgauge(root, bootstyle="dark", maximum=self.run_duration_var.get(), value=0, 
                                               text="Not Running",
                                               mode="determinate") #, 
                                            #   font=self.text_font) 
        self.run_progress_bar.grid(row=2, column=4, columnspan=2, sticky="ew", padx=20, pady=0)

        # Buttons
        self.button_width = 15
        self.start_daq_button = ttk.Button(root, text="Start DAQ", command=self.start_daq, 
                                           width=self.button_width, bootstyle="success")
        self.start_daq_button.grid(row=1, column=0, sticky="nsew", padx=20, pady=10)

        self.stop_daq_button = ttk.Button(root, text="! Stop DAQ", command=self.stop_daq, 
                                          width=self.button_width, bootstyle="danger", state="disabled")
        self.stop_daq_button.grid(row=1, column=5, sticky="nsew", padx=20, pady=10)

        self.start_button = ttk.Button(root, text="Start Run", command=self.start_run, 
                                       width=self.button_width, bootstyle="success", state="disabled")
        self.start_button.grid(row=3, column=0, sticky="nsew", padx=20, pady=10)

        self.stop_button = ttk.Button(root, text="Stop Run", command=self.stop_run, 
                                      width=self.button_width, bootstyle="danger", state="disabled")
        self.stop_button.grid(row=3, column=1, sticky="nsew", padx=20, pady=10)

        self.pause_button = ttk.Button(root, text="Pause", command=self.pause_daq, 
                                       width=self.button_width, bootstyle="warning", state="disabled")
        
        self.pause_button.grid(row=3, column=2, sticky="nsew", padx=20, pady=10)

        self.resume_button = ttk.Button(root, text="Resume", command=self.resume_daq, 
                                        width=self.button_width, bootstyle="success", state="disabled")
        
        self.resume_button.grid(row=3, column=3, columnspan = 1, sticky="n", padx=20, pady=10)

        # Live analysis checkbox
        self.live_analysis_var = ttk.BooleanVar()
        self.live_analysis_checkbox = ttk.Checkbutton(root, text=" ", variable=self.live_analysis_var, 
                                                      command=self.toggle_live_analysis, bootstyle="success-round-toggle", state="disabled")
        self.live_analysis_checkbox.grid(row=4, column=0, sticky="ne", padx=20, pady=10)
        self.live_analysis_label = ttk.Label(root, text="Live Analysis", font=self.label_font, bootstyle="secondary")
        self.live_analysis_label.grid(row=4, column=0, sticky="nw", padx=40, pady=10)

        self.event_times = []
        self.seen_event_ids = set()

        # Create a matplotlib figure
        self.fig, self.ax = plt.subplots(figsize=(6, 4))
        self.fig.patch.set_alpha(0.0)
        #self.line, = self.ax.plot([], [], lw=1)
        self.line, = self.ax.plot([], [], marker='.', linestyle='-')
        self.ax.set_xlabel("TDC Time Tag (s)")
        self.ax.set_ylabel("Events")
        self.ax.set_xlim(0, self.run_duration_var.get())

        # Embed the matplotlib plot in tkinter, in grid row 5
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)  # or your specific parent frame
        self.canvas_widget = self.canvas.get_tk_widget()
        self.canvas_widget.grid(row=5, column=0, columnspan=5, padx=30, pady=20)  # adjust columns as needed
        self.fig.tight_layout()

        self.canvas.draw()



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
        self.stop_daq_button.config(state="disabled")
        self.start_daq_button.config(state="disabled")
        self.run_running = True
        self.update_floodgauge()
        self.update_run_number()
        lockfile = f"./tmp/hodo_run_{self.run_number}.lock"
        open(lockfile, "w").close()
        self.clear_plot()
        self.toggle_live_analysis()

    def stop_run(self):
        self.send_command("stop")
        self.start_button.config(state="normal")
        self.stop_button.config(state="disabled")
        self.stop_daq_button.config(state="normal")
        self.run_running = False
        self.run_progress_bar["value"] = 0
        self.run_progress_bar["text"] = "Not Running"
        lockfile = f"./tmp/hodo_run_{self.run_number}.lock"
        if os.path.exists(lockfile):
            os.remove(lockfile)
        self.save_plot(f"run_{self.run_number:0>5}_{self.cusp_number:0>4}")
        self.clear_plot()

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
            self.run_progress_bar.configure(text="Not Running", bootstyle="dark")

        if self.daq_process:
            self.send_command("!")
            self.start_button.config(state="disabled")
            self.stop_button.config(state="disabled")
            self.pause_button.config(state="disabled")
            self.resume_button.config(state="disabled")
            self.start_daq_button.config(state="normal")
            self.auto_run_checkbox.config(state="disabled")
            self.auto_run_label.config(bootstyle="success")
            self.auto_run_enabled = False
            self.daq_process.terminate()
            self.daq_process = None
    
    def start_daq(self):
        if self.daq_process is None or self.daq_process.poll() is not None:
            geometry = f"80x24+1140+100"  #"120x24+600+1200"
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
                #self.console.write(f"CUSP number: {cusp["cusp_run"]}", "SUCCESS")
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
            #self.stop_button.config(state="disabled")
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
        self.current_cusp_run = self.read_cusp_run_number().get("cusp_run")
        while self.auto_run_enabled:
            new_cusp_run = self.read_cusp_run_number().get("cusp_run")
            self.cusp_number_label.config(text=f"CUSP Run: {self.cusp_number}")

            if new_cusp_run and new_cusp_run != self.current_cusp_run:
                self.console.write(f"CUSP number: {self.cusp_number}", "SUCCESS")
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
        if self.live_analysis_var.get() and self.run_running:
            self.live_analysis_enabled = True
            threading.Thread(target=self.live_analysis_monitor, daemon=True).start()
        elif self.live_analysis_var.get() and not self.run_running:
            self.live_analysis_enabled = True
        else:
            self.live_analysis_enabled = False


    def live_analysis_monitor(self):
        """Starts analysis and waits for incoming data."""
        context = zmq.Context()
        socket = context.socket(zmq.SUB)
        socket.connect("tcp://localhost:5555")
        socket.setsockopt_string(zmq.SUBSCRIBE, "") 
        geometry = f"80x24+1140+300"
        self.live_process = subprocess.Popen(["gnome-terminal", f"--geometry={geometry}", 
                                              "--title=Live Analysis Terminal", "--", "bash", "-c", 
                                              f"cd data_analysis/build; ./hodo_analysis -l {str(self.run_number)}; exec bash"])

        try:
            while self.live_analysis_enabled:
                try:
                    message = socket.recv_string(flags=zmq.NOBLOCK)
                    self.process_live_data(message)
                except zmq.Again:
                    time.sleep(0.1)  # Avoid busy wait
        except Exception as e:
            self.console.write("Live analysis error:", e)
        finally:
            socket.close()
            context.term()
            if self.live_process.poll() is None:
                self.live_process.terminate()

    def process_live_data(self, message):
        
        tokens = message.strip().split()
        event_id = int(tokens[0])
        tdc_time = float(tokens[1])*1e-9
        active_channels = list(map(int, tokens[2:]))
        self.console.write(f"Event {event_id}, Time {tdc_time}, Channels: {active_channels}")

        if event_id in self.seen_event_ids:
            return 
        self.seen_event_ids.add(event_id)

        self.event_times.append([tdc_time, event_id])
        self.update_plot()


    def update_plot(self):
        if not self.event_times:
            return
        tdc_times = np.array(self.event_times)[:,0]
        tdc_event = np.arange(1, len(self.event_times) + 1) # np.array(self.event_times)[:,1]

        self.line.set_data(tdc_times, tdc_event)
        self.ax.relim()
        self.ax.autoscale_view()
        # self.ax.plot(tdc_times, tdc_event, marker='.', linestyle='-')
        self.ax.set_xlabel("TDC Time in s")
        self.ax.set_ylabel("Number of Events")
        self.ax.set_xlim(0, self.run_duration_var.get())
        self.ax.grid(True)

        self.fig.tight_layout()
        self.canvas.draw()

    def clear_plot(self):
        self.event_times.clear()
        self.line.set_data([], [])
        self.ax.tick_params(colors='white')  # Tick label color
        self.ax.xaxis.label.set_color('lightgray')
        self.ax.yaxis.label.set_color('lightgray')
        for spine in self.ax.spines.values():
            spine.set_edgecolor("white")
        self.ax.relim()
        self.ax.autoscale_view()
        self.ax.set_xlim(0, self.run_duration_var.get())
        self.canvas.draw()

    def save_plot(self, filename_base="tdc_plot"):
        self.ax.tick_params(colors='#161616')  # Tick label color
        self.ax.xaxis.label.set_color('#161616')
        self.ax.yaxis.label.set_color('#161616')
        for spine in self.ax.spines.values():
            spine.set_edgecolor("#161616")

        # Save as PNG and PDF
        self.fig.savefig(f"{self.config.get("daq_path")}/data/plots/{filename_base}.png", dpi=300, bbox_inches='tight')
        self.fig.savefig(f"{self.config.get("daq_path")}/data/plots/{filename_base}.pdf", bbox_inches='tight')

        #print(f"Plot saved as {filename_base}.png and .pdf")

# Run the application
if __name__ == "__main__":
    root = ttk.Window(themename="superhero")  # Dark mode theme
    app = DAQControllerApp(root)
    # Intercept the close button (X) event
    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()