import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import ROOT
import sys
import os
from matplotlib.patches import Rectangle
from matplotlib.colors import Normalize
from mpl_toolkits.axes_grid1 import make_axes_locatable
from collections import Counter

# Setup for plotting
plt.style.use("ggplot")
plt.set_cmap("viridis")
plt.rcParams["grid.linestyle"] = "--"
plt.rcParams["grid.color"] = "lightgray"
plt.rcParams["grid.linewidth"] = 0.8
plt.rcParams["axes.axisbelow"] = True
plt.rcParams["font.family"] = "sans-serif"
plt.rcParams.update({'font.size': 8})
plt.rcParams['pdf.fonttype'] = 'truetype'
plt.rcParams["axes.prop_cycle"] = plt.cycler(color=["#316D97", "#DD5043", "#65236E", "#6D904F", "#FFC636", "#8b8b8b",  # ASACUSA colors
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
plt.rcParams["text.color"] = "161616"
plt.rcParams["legend.labelcolor"] = "#161616"
plt.rcParams["legend.facecolor"] = "white"
plt.rcParams["axes.labelcolor"] = "161616"
plt.rcParams["axes.titlecolor"] = "161616"
plt.rcParams["figure.facecolor"] = "#ffffff"
plt.rcParams['pdf.fonttype'] = 'truetype'
plt.rcParams["figure.dpi"] = 150
plt.rcParams["savefig.facecolor"] = (1.0, 1.0, 1.0, 0.0)
px = 1/plt.rcParams['figure.dpi']



if len(sys.argv) == 2:
    # print(sys.argv)
    run = int(sys.argv[1])
else:
    print("Usage: ./create_plots.py RUN_NR")
    exit()


class Data:
    def __init__(self, run_nr):
        self.run_nr = run_nr
        self.config_file = "../../config/daq_config.conf"  
        self.config = self.read_config()

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
            print(f"Warning: {self.config_file} not found.", "WARNING")
        return config
    
    def read_root(self):
        self.file_path = f'{self.config.get("daq_path")}{self.config.get("ana_path")}/{self.config.get("ana_prefix")}{self.run_nr:06}.root'
        if os.path.exists(self.file_path):
            print(self.file_path, "exists")
        else: 
            print(self.file_path, "does not exist!")
            exit()
        # file = ROOT.TFile.Open(self.file_path, "READ")
        # tree = file.Get("EventTree")

        rdf = ROOT.RDataFrame("EventTree", self.file_path)
        try:
            dat = rdf.AsNumpy(["eventID", "tdcTimeTag", "mixGate", "fpgaTimeTag", "bgo_Channels", "cuspRunNumber"])
            df = pd.DataFrame( {
                "events": pd.Series(dat["eventID"], dtype=np.dtype("uint32")),
                # "times":  pd.Series(dat["tdcTimeTag"], dtype=np.dtype("float")),
                "times": pd.Series(dat["fpgaTimeTag"], dtype=np.dtype("float")),
                "gates":  pd.Series(dat["mixGate"], dtype=np.dtype("bool")),
                "channels": [np.array(v).tolist() for v in dat["bgo_Channels"]]
            } ) #, columns=["events", "times", "mixGate", "bgo_Channels"], )
        except:
            dat = rdf.AsNumpy(["eventID", "tdcTimeTag", "mixGate", "bgo_Channels", "cuspRunNumber"])
            df = pd.DataFrame( {
                "events": pd.Series(dat["eventID"], dtype=np.dtype("uint32")),
                "times":  pd.Series(dat["tdcTimeTag"], dtype=np.dtype("float")),
                "gates":  pd.Series(dat["mixGate"], dtype=np.dtype("bool")),
                "channels": [np.array(v).tolist() for v in dat["bgo_Channels"]]
        } ) #, columns=["events", "times", "mixGate", "bgo_Channels"], )
        # df["channels"] = [np.array(v).tolist() for v in dat["bgo_Channels"]]
        # print(df.head(20))
        # print(df.dtypes)
        print(df)
        df["mix_events"] =(df["gates"].cumsum() * df["gates"]).mask(~df["gates"]).ffill().fillna(0).astype(int)
        # df["gates_inv"] = ~df["gates"]
        # df["mix_events"] =(df["gates_inv"].cumsum() * df["gates_inv"]).mask(~df["gates_inv"]).ffill().fillna(0).astype(int)

        self.events = np.arange(len(df))+1
        self.times = df.times
        self.mix_events = df.mix_events
        self.channels = df.channels

        self.cusp_nr = Counter(dat["cuspRunNumber"]).most_common(1)[0][0]
        
        # print(df.head(40))






class Plotter:
    def __init__(self, times, events, mix_events, channels):
        self.times = times
        self.events = events
        self.mix_events = mix_events
        self.channels = channels    
    
    def event_plot(self):

        # Create the event plot
        self.fig1, self.ax1 = plt.subplots(figsize=(6,4))
        self.fig1.patch.set_alpha(0.0)
        self.line, = self.ax1.plot(self.times*1e-9, self.events, marker='.', linestyle='-', markersize=3, label=f"Events: {np.max(self.events)}")
        self.line2, = self.ax1.plot(self.times*1e-9, self.mix_events, marker='.', linestyle='-', markersize=3, label=f"Mixing Events: {np.max(self.mix_events)}", color="C2")
        self.ax1.legend()
        self.ax1.tick_params(colors='#161616')
        self.ax1.set_xlabel("TDC Time Tag (s)")
        self.ax1.set_ylabel("Events")
        # self.ax1.set_xlim(0, self.run_duration_var.get())
        for spine in self.ax1.spines.values():
            spine.set_edgecolor("#161616")
        # plt.show()



    def bgo_plot(self, data):
        # # Needed for BGO geometry:
        self.bgo_geom = pd.read_csv(f"{data.config.get("daq_path")}/config/bgo_geom.csv")
        self.ch_w = 10
        self.ch_h = 5

        # Data for BGO plots
        self.rectangles = []
        #self.BGO_counts = np.zeros(64)


        all_channels = [ch for sublist in self.channels for ch in sublist]
        self.BGO_counts = np.bincount(all_channels, minlength=64)
        # print(self.BGO_counts)


        # Needed for colorbar:
        self.cmap = plt.cm.viridis
        self.norm = Normalize(vmin=0, vmax=max(self.BGO_counts) if max(self.BGO_counts) > 1 else 1)
        self.zero_color = "#ececec"

        # # Create the BGO plot
        self.fig2, self.ax2 = plt.subplots(figsize=(4,4))
        self.fig2.patch.set_alpha(0.0)
        # Drawing a circle of the size of the BGO
        self.circle, = self.ax2.fill(45*np.cos(np.linspace( 0, 2*np.pi, 150)), 45*np.sin(np.linspace( 0, 2*np.pi, 150)), 
                                     linestyle='--', ec="white", fc="lightgray", alpha=0.4)
        # Creating 64 rectangles for the BGO channels
        for x, y, c in zip(self.bgo_geom.x, self.bgo_geom.y, self.BGO_counts): 
            if c == 0:
                color = self.zero_color
            else:
                color = self.cmap(self.norm(c))
            rect = Rectangle(xy=(x - self.ch_w/2, y - self.ch_h/2),
                            width=self.ch_w, height=self.ch_h,
                            color=color)
            self.ax2.add_artist(rect)
            self.rectangles.append(rect)
        
        self.ax2.tick_params(colors='#161616')
        for spine in self.ax2.spines.values():
            spine.set_edgecolor("#161616")
        self.ax2.set_aspect('equal')
        self.ax2.set_xlabel("x (mm)")
        self.ax2.set_ylabel("y (mm)")

        # Setting up the colorbar, also making sure it's the same size as the plot: 
        self.sm = plt.cm.ScalarMappable(norm=self.norm, cmap='viridis')
        divider = make_axes_locatable(self.ax2)
        cax = divider.append_axes("right", size="5%", pad=0.05)  # Adjust size and padding
        self.cbar = self.fig2.colorbar(self.sm, cax=cax)
        self.cbar.set_label("Counts")
        self.cbar.ax.yaxis.set_tick_params(color='#161616')  # Tick marks
        plt.setp(self.cbar.ax.get_yticklabels(), color='#161616')  # Tick text
        self.cbar.set_label("Counts", color='#161616')  # Set color of the label
        for spine in self.cbar.ax.spines.values():
            spine.set_edgecolor('#161616')

        # plt.show()

    
    def save_plot(self, figure, filename="tdc_events"):
        """Saves the plots once as png and once as pdf, text and outlines are set to black"""
        # Save as PNG and PDF
        figure.savefig(f"{filename}.png", dpi=300, bbox_inches='tight')
        figure.savefig(f"{filename}.pdf", bbox_inches='tight')

    def save_tmp_plot(self, figure, filename ="tmp_plot"):
        try:
            self.ax1.tick_params(colors='lightgray')
            for spine in self.ax1.spines.values():
                spine.set_edgecolor("lightgray")
            self.ax1.set_xlabel("TDC Time Tag (s)", color='lightgray')
            self.ax1.set_ylabel("Events", color='lightgray')
        except:
            print("")
        try:
            self.ax2.tick_params(colors='lightgray')
            for spine in self.ax2.spines.values():
                spine.set_edgecolor("lightgray")
            self.ax2.set_xlabel("x (mm)", color='lightgray')
            self.ax2.set_ylabel("y (mm)", color='lightgray')
            self.cbar.ax.yaxis.set_tick_params(color='lightgray')  # Tick marks
            plt.setp(self.cbar.ax.get_yticklabels(), color='lightgray')  # Tick text
            self.cbar.set_label("Counts", color='lightgray')  # Set color of the label
            for spine in self.cbar.ax.spines.values():
                spine.set_edgecolor('lightgray')
        except:
            print("")
        figure.savefig(f"{filename}.png", dpi=300, bbox_inches='tight')



def main():
    data = Data(run)
    data.read_root()

    plot = Plotter(data.times, data.events, data.mix_events, data.channels)
    plot.event_plot()
    plot.save_plot(plot.fig1, f"{data.config.get("daq_path")}/data/plots/{data.cusp_nr}_run_{data.run_nr:0>5}_events")
    plot.save_tmp_plot(plot.fig1, f"{data.config.get("daq_path")}/data/plots/tmp_events")
    plot.bgo_plot(data)
    plot.save_plot(plot.fig2, f"{data.config.get("daq_path")}/data/plots/{data.cusp_nr}_run_{data.run_nr:0>5}_bgo")
    plot.save_tmp_plot(plot.fig2, f"{data.config.get("daq_path")}/data/plots/tmp_bgo")


if __name__=="__main__":
    main()