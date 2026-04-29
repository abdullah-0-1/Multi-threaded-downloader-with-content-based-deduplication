
"""
Distributed Download Manager - GUI Client
Professional interface for the deduplication-based download system
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from download_tab import DownloadTab
from upload_tab import UploadTab
from settings_tab import SettingsTab

class DownloadManagerGUI:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Distributed Download Manager - Deduplication System")
        self.root.geometry("1000x700")
        self.root.minsize(900, 650)
        
        try:
            self.root.iconphoto(False, tk.PhotoImage(file='ui/icon.png'))
        except:
            pass
        

        self.setup_style()
        
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(1, weight=1)
        
        self.create_header(main_frame)
        

        self.notebook = ttk.Notebook(main_frame)
        self.notebook.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(10, 0))
        
        self.download_tab = DownloadTab(self.notebook)
        self.upload_tab = UploadTab(self.notebook)
        self.settings_tab = SettingsTab(self.notebook)
        
        self.notebook.add(self.download_tab, text="  📥 Download  ")
        self.notebook.add(self.upload_tab, text="  📤 Upload  ")
        self.notebook.add(self.settings_tab, text="  ⚙️ Settings  ")
        
        self.create_status_bar(main_frame)
        
        self.center_window()
        
    def setup_style(self):
        """Configure ttk styles for professional look"""
        style = ttk.Style()
        
        available_themes = style.theme_names()
        if 'clam' in available_themes:
            style.theme_use('clam')
        elif 'alt' in available_themes:
            style.theme_use('alt')
        
        bg_color = '#f5f6fa'
        accent_color = '#3498db'
        success_color = '#27ae60'
        text_color = '#2c3e50'
        
        self.root.configure(bg=bg_color)
        
        style.configure('Header.TLabel', 
                       font=('Helvetica', 18, 'bold'), 
                       foreground=text_color,
                       background=bg_color)
        style.configure('Subheader.TLabel', 
                       font=('Helvetica', 10), 
                       foreground='#7f8c8d',
                       background=bg_color)
        
        style.configure('TFrame', background=bg_color)
        style.configure('TLabelframe', background=bg_color, foreground=text_color)
        style.configure('TLabelframe.Label', font=('Helvetica', 10, 'bold'))
        
        style.configure('TNotebook', background=bg_color, borderwidth=0)
        style.configure('TNotebook.Tab', 
                       font=('Helvetica', 10, 'bold'),
                       padding=[20, 10])
        style.map('TNotebook.Tab',
                 background=[('selected', accent_color), ('!selected', '#ecf0f1')],
                 foreground=[('selected', 'white'), ('!selected', text_color)])
        
        style.configure('TButton', 
                       font=('Helvetica', 10),
                       padding=[10, 5])
        style.map('TButton',
                 background=[('active', accent_color), ('!active', '#ecf0f1')],
                 foreground=[('active', 'white')])
        
        style.configure('Green.Horizontal.TProgressbar',
                       background=success_color,
                       troughcolor='#ecf0f1',
                       bordercolor=bg_color,
                       lightcolor=success_color,
                       darkcolor=success_color)
        style.configure('Status.TLabel', font=('Helvetica', 9), foreground='#34495e')
        
        style.configure('Primary.TButton', font=('Helvetica', 10, 'bold'))
        style.configure('Success.TButton', font=('Helvetica', 10))
        
    def create_header(self, parent):
        """Create application header"""
        header_frame = ttk.Frame(parent)
        header_frame.grid(row=0, column=0, sticky=(tk.W, tk.E))
        
        title_label = ttk.Label(
            header_frame, 
            text="Download System",
            style='Header.TLabel'
        )
        title_label.grid(row=0, column=0, sticky=tk.W)
        
        subtitle_label = ttk.Label(
            header_frame,
            text="Multi-threaded downloader with content-based deduplication • B-Tree indexing • HashMap caching",
            style='Subheader.TLabel'
        )
        subtitle_label.grid(row=1, column=0, sticky=tk.W, pady=(2, 0))
        
    def create_status_bar(self, parent):
        """Create status bar at bottom"""
        status_frame = ttk.Frame(parent, relief=tk.SUNKEN)
        status_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=(10, 0))
        
        self.status_label = ttk.Label(
            status_frame,
            text="Ready",
            style='Status.TLabel'
        )
        self.status_label.pack(side=tk.LEFT, padx=5, pady=2)
        
        self.server_status = ttk.Label(
            status_frame,
            text="● Server: Checking...",
            style='Status.TLabel',
            foreground='orange'
        )
        self.server_status.pack(side=tk.RIGHT, padx=5, pady=2)
        
        self.root.after(1000, self.check_server_status)
        
    def check_server_status(self):
        """Check if server is reachable"""
        import socket
        import json
        
        # Load settings to get server IP and port
        settings_file = os.path.join(os.path.dirname(__file__), 'settings.json')
        server_ip = '127.0.0.1'
        server_port = 8080
        
        try:
            if os.path.exists(settings_file):
                with open(settings_file, 'r') as f:
                    settings = json.load(f)
                    server_ip = settings.get('server_ip', '127.0.0.1')
                    server_port = settings.get('server_port', 8080)
        except:
            pass
        
        # Test connection
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2)
            result = sock.connect_ex((server_ip, server_port))
            sock.close()
            
            if result == 0:
                self.server_status.config(
                    text="● Server: Connected",
                    foreground='green'
                )
            else:
                self.server_status.config(
                    text="● Server: Offline",
                    foreground='red'
                )
        except:
            self.server_status.config(
                text="● Server: Offline",
                foreground='red'
            )
        
        self.root.after(10000, self.check_server_status)
    
    def center_window(self):
        """Center the window on screen"""
        self.root.update_idletasks()
        width = self.root.winfo_width()
        height = self.root.winfo_height()
        x = (self.root.winfo_screenwidth() // 2) - (width // 2)
        y = (self.root.winfo_screenheight() // 2) - (height // 2)
        self.root.geometry(f'{width}x{height}+{x}+{y}')
    
    def update_status(self, message):
        """Update status bar message"""
        self.status_label.config(text=message)
    
    def run(self):
        """Start the GUI application"""
        self.root.mainloop()

def main():
    """Main entry point"""
    client_path = os.path.join(os.path.dirname(__file__), '..', 'client_app')
    if not os.path.exists(client_path):
        messagebox.showerror(
            "Error",
            "Client application not found!\n\n"
            "Please build the project first:\n"
            "./build_all.sh"
        )
        return
    
    app = DownloadManagerGUI()
    app.run()

if __name__ == "__main__":
    main()
