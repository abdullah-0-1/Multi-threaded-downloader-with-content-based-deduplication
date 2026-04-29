"""
Settings Tab - Configure application settings
"""

import tkinter as tk
from tkinter import ttk, messagebox
import os
import json

class SettingsTab(ttk.Frame):
    def __init__(self, parent):
        super().__init__(parent, padding="20")
        
        self.settings = {
            'server_ip': '127.0.0.1',
            'server_port': 8080,
            'threads': 8,
            'chunk_size': 256
        }
        
        self.load_settings()
        
        self.columnconfigure(0, weight=1)
        
        self.create_widgets()
    
    def create_widgets(self):
        """Create all widgets for settings tab"""
        row = 0
        
        server_frame = ttk.LabelFrame(self, text="Server Configuration", padding="15")
        server_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 15))
        server_frame.columnconfigure(1, weight=1)
        
        ttk.Label(server_frame, text="Server IP:").grid(row=0, column=0, sticky=tk.W, padx=(0, 10), pady=5)
        self.ip_entry = ttk.Entry(server_frame, width=30)
        self.ip_entry.grid(row=0, column=1, sticky=tk.W, pady=5)
        self.ip_entry.insert(0, self.settings['server_ip'])
        
        ttk.Label(server_frame, text="Server Port:").grid(row=1, column=0, sticky=tk.W, padx=(0, 10), pady=5)
        self.port_entry = ttk.Entry(server_frame, width=30)
        self.port_entry.grid(row=1, column=1, sticky=tk.W, pady=5)
        self.port_entry.insert(0, str(self.settings['server_port']))
        
        ttk.Label(
            server_frame,
            text="Configure the deduplication server connection",
            font=('Helvetica', 9),
            foreground='#7f8c8d'
        ).grid(row=2, column=0, columnspan=2, sticky=tk.W, pady=(5, 0))
        
        row += 1
    
        download_frame = ttk.LabelFrame(self, text="Download Settings", padding="15")
        download_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 15))
        download_frame.columnconfigure(1, weight=1)
        
        ttk.Label(download_frame, text="Thread Count:").grid(row=0, column=0, sticky=tk.W, padx=(0, 10), pady=5)
        self.threads_spinbox = ttk.Spinbox(download_frame, from_=1, to=16, width=28)
        self.threads_spinbox.grid(row=0, column=1, sticky=tk.W, pady=5)
        self.threads_spinbox.set(self.settings['threads'])
        
        ttk.Label(download_frame, text="Chunk Size (KB):").grid(row=1, column=0, sticky=tk.W, padx=(0, 10), pady=5)
        self.chunk_spinbox = ttk.Spinbox(download_frame, from_=64, to=1024, increment=64, width=28)
        self.chunk_spinbox.grid(row=1, column=1, sticky=tk.W, pady=5)
        self.chunk_spinbox.set(self.settings['chunk_size'])
        
        ttk.Label(
            download_frame,
            text="More threads = faster download (if server supports range requests)",
            font=('Helvetica', 9),
            foreground='#7f8c8d'
        ).grid(row=2, column=0, columnspan=2, sticky=tk.W, pady=(5, 0))
        
        row += 1
        
        storage_frame = ttk.LabelFrame(self, text="Storage Locations", padding="15")
        storage_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 15))
        storage_frame.columnconfigure(1, weight=1)
        
        script_dir = os.path.dirname(os.path.abspath(__file__))
        downloads_path = os.path.abspath(os.path.join(script_dir, '..', 'downloads'))
        
        ttk.Label(storage_frame, text="Downloads Folder:").grid(row=0, column=0, sticky=tk.W, padx=(0, 10), pady=5)
        ttk.Label(
            storage_frame,
            text=downloads_path,
            font=('Courier', 9),
            foreground='#2c3e50'
        ).grid(row=0, column=1, sticky=tk.W, pady=5)
        
        row += 1
        
        info_frame = ttk.LabelFrame(self, text="System Information", padding="15")
        info_frame.grid(row=row, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 15))
        info_frame.rowconfigure(0, weight=1)
        info_frame.columnconfigure(0, weight=1)
        
        info_text = tk.Text(
            info_frame,
            height=10,
            font=('Courier', 9),
            bg='#ecf0f1',
            fg='#2c3e50',
            state='disabled',
            wrap='word'
        )
        info_text.pack(fill=tk.BOTH, expand=True)
        
        project_root = os.path.abspath(os.path.join(script_dir, '..'))
        
        info = "=== Distributed Download Manager ===\n\n"
        info += "Components:\n"
        info += f"  • Multi-threaded Downloader (IDM): {'✓ Built' if os.path.exists(os.path.join(project_root, 'idm')) else '✗ Not found'}\n"
        info += f"  • Client Application: {'✓ Built' if os.path.exists(os.path.join(project_root, 'client_app')) else '✗ Not found'}\n"
        info += f"  • Server Application: {'✓ Built' if os.path.exists(os.path.join(project_root, 'server_app')) else '✗ Not found'}\n\n"
        
        info += "Data Structures:\n"
        info += "  • Custom HashMap (DJB2 hash, separate chaining)\n"
        info += "  • B-Tree Index (degree 3, O(log n) search)\n"
        info += "  • Min-Heap Priority Queue (chunk scheduling)\n"
        info += "  • Block-based Chunk Index (O(1) access)\n\n"
        
        info += "Features:\n"
        info += "  • Multi-threaded parallel downloads\n"
        info += "  • Content-based deduplication (SHA-256)\n"
        info += "  • Client-server architecture\n"
        info += "  • Persistent storage with atomic saves\n"
        info += "  • Cross-file deduplication\n"
        
        info_text.config(state='normal')
        info_text.insert(1.0, info)
        info_text.config(state='disabled')
        
        row += 1
        
        button_frame = ttk.Frame(self)
        button_frame.grid(row=row, column=0, sticky=(tk.W, tk.E))
        
        ttk.Button(
            button_frame,
            text="Save Settings",
            command=self.save_settings,
            style='Primary.TButton',
            width=20
        ).pack(side=tk.LEFT, padx=(0, 10))
        
        ttk.Button(
            button_frame,
            text="Reset to Defaults",
            command=self.reset_settings,
            width=20
        ).pack(side=tk.LEFT)
        
        ttk.Button(
            button_frame,
            text="Test Server Connection",
            command=self.test_connection,
            width=25
        ).pack(side=tk.RIGHT)
    
    def load_settings(self):
        """Load settings from file"""
        script_dir = os.path.dirname(os.path.abspath(__file__))
        settings_file = os.path.join(script_dir, 'settings.json')
        
        if os.path.exists(settings_file):
            try:
                with open(settings_file, 'r') as f:
                    self.settings = json.load(f)
            except:
                pass
    
    def save_settings(self):
        """Save current settings"""
        try:
            self.settings['server_ip'] = self.ip_entry.get()
            self.settings['server_port'] = int(self.port_entry.get())
            self.settings['threads'] = int(self.threads_spinbox.get())
            self.settings['chunk_size'] = int(self.chunk_spinbox.get())
            
            script_dir = os.path.dirname(os.path.abspath(__file__))
            settings_file = os.path.join(script_dir, 'settings.json')
            
            with open(settings_file, 'w') as f:
                json.dump(self.settings, f, indent=2)
            
            messagebox.showinfo("Success", "Settings saved successfully!")
            
        except ValueError as e:
            messagebox.showerror("Error", "Invalid input. Please check your values.")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save settings:\n{str(e)}")
    
    def reset_settings(self):
        """Reset to default settings"""
        self.ip_entry.delete(0, tk.END)
        self.ip_entry.insert(0, '127.0.0.1')
        
        self.port_entry.delete(0, tk.END)
        self.port_entry.insert(0, '8080')
        
        self.threads_spinbox.set(8)
        self.chunk_spinbox.set(256)
        
        messagebox.showinfo("Reset", "Settings reset to defaults")
    
    def test_connection(self):
        """Test connection to server"""
        import socket
        
        try:
            server_ip = self.ip_entry.get()
            server_port = int(self.port_entry.get())
            
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(3)
            result = sock.connect_ex((server_ip, server_port))
            sock.close()
            
            if result == 0:
                messagebox.showinfo(
                    "Connection Test",
                    f"✓ Successfully connected to server!\n\n"
                    f"Server: {server_ip}:{server_port}\n"
                    f"Status: Online"
                )
            else:
                messagebox.showwarning(
                    "Connection Test",
                    f"✗ Could not connect to server\n\n"
                    f"Server: {server_ip}:{server_port}\n"
                    f"Status: Offline\n\n"
                    f"Make sure the server is running:\n"
                    f"./server_app"
                )
        except ValueError:
            messagebox.showerror("Error", "Invalid port number")
        except Exception as e:
            messagebox.showerror("Error", f"Connection test failed:\n{str(e)}")
