"""
Download Tab - Handle downloading files from internet or server
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import subprocess
import threading
import re
import os

class DownloadTab(ttk.Frame):
    def __init__(self, parent):
        super().__init__(parent, padding="20")
        
        self.is_downloading = False
        self.current_process = None
        
        self.columnconfigure(0, weight=1)
        
        self.create_widgets()
        
    def create_widgets(self):
        """Create all widgets for download tab"""
        row = 0
        
        url_frame = ttk.LabelFrame(self, text=" Download Source", padding="15")
        url_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 15))
        url_frame.columnconfigure(1, weight=1)
        
        ttk.Label(url_frame, 
                 text="URL:", 
                 font=('Helvetica', 10, 'bold')).grid(row=0, column=0, sticky=tk.W, padx=(0, 10))
        
        self.url_entry = ttk.Entry(url_frame, font=('Consolas', 10))
        self.url_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(0, 10))
        self.url_entry.insert(0, "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4")
        
        ttk.Button(
            url_frame,
            text="📁 Browse",
            command=self.browse_url_history,
            width=12
        ).grid(row=0, column=2)
        
        helper_label = ttk.Label(
            url_frame, 
            text="Tip: For uploaded files, use format: local://filename.ext",
            font=('Helvetica', 9, 'italic'),
            foreground='#666666'
        )
        helper_label.grid(row=1, column=0, columnspan=3, sticky=tk.W, pady=(5, 0))
        
        row += 1
        
        mode_frame = ttk.LabelFrame(self, text="⚡ Download Mode", padding="15")
        mode_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 15))
        
        self.download_mode = tk.StringVar(value="smart")
        
        ttk.Radiobutton(
            mode_frame,
            text="Smart Mode (Check server first, then internet)",
            variable=self.download_mode,
            value="smart"
        ).grid(row=0, column=0, sticky=tk.W, pady=8)
        
        ttk.Radiobutton(
            mode_frame,
            text=" Server Only (Download from deduplication server)",
            variable=self.download_mode,
            value="server"
        ).grid(row=1, column=0, sticky=tk.W, pady=8)
        
        ttk.Radiobutton(
            mode_frame,
            text="Internet Only (Direct download, no server interaction)",
            variable=self.download_mode,
            value="internet"
        ).grid(row=2, column=0, sticky=tk.W, pady=8)
        
        row += 1
        
        progress_frame = ttk.LabelFrame(self, text="📊 Download Progress", padding="15")
        progress_frame.grid(row=row, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 15))
        progress_frame.columnconfigure(0, weight=1)
        progress_frame.rowconfigure(3, weight=1)
        
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(
            progress_frame,
            style='Green.Horizontal.TProgressbar',
            mode='determinate',
            variable=self.progress_var,
            length=400
        )
        self.progress_bar.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        self.status_label = ttk.Label(
            progress_frame,
            text="Ready to download",
            font=('Helvetica', 10)
        )
        self.status_label.grid(row=1, column=0, sticky=tk.W, pady=(0, 5))
        
        self.details_label = ttk.Label(
            progress_frame,
            text="",
            font=('Courier', 9),
            foreground='#7f8c8d'
        )
        self.details_label.grid(row=2, column=0, sticky=tk.W, pady=(0, 10))
        
        console_label = ttk.Label(progress_frame, 
                                 text="Console Output:",
                                 font=('Helvetica', 10, 'bold'))
        console_label.grid(row=3, column=0, sticky=(tk.W, tk.N), pady=(5, 5))
        
        text_frame = ttk.Frame(progress_frame)
        text_frame.grid(row=4, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        text_frame.columnconfigure(0, weight=1)
        text_frame.rowconfigure(0, weight=1)
        
        self.console_text = tk.Text(
            text_frame,
            height=10,
            width=80,
            font=('Consolas', 9),
            bg='#1e272e',
            fg='#00d2d3',
            insertbackground='#00d2d3',
            selectbackground='#3498db',
            selectforeground='white',
            state='disabled',
            wrap='word',
            borderwidth=2,
            relief='flat',
            padx=8,
            pady=8
        )
        self.console_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        scrollbar = ttk.Scrollbar(text_frame, orient='vertical', command=self.console_text.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.console_text['yscrollcommand'] = scrollbar.set
        
        row += 1
        
        button_frame = ttk.Frame(self)
        button_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 0))
        
        self.download_btn = ttk.Button(
            button_frame,
            text="Start Download",
            command=self.start_download,
            style='Primary.TButton',
            width=22
        )
        self.download_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        self.cancel_btn = ttk.Button(
            button_frame,
            text="⏹  Cancel",
            command=self.cancel_download,
            state='disabled',
            width=15
        )
        self.cancel_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        ttk.Button(
            button_frame,
            text="Clear Console",
            command=self.clear_console,
            width=17
        ).pack(side=tk.LEFT)
        
        ttk.Button(
            button_frame,
            text="Open Downloads",
            command=self.open_downloads_folder,
            width=20
        ).pack(side=tk.RIGHT)
        
    def browse_url_history(self):
        """Show common URLs or allow file selection"""
        messagebox.showinfo(
            "URL History",
            "Recent URLs:\n\n" +
            "• http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4\n" +
            "• https://www.w3.org/WAI/ER/tests/xhtml/testfiles/resources/pdf/dummy.pdf"
        )
    
    def append_console(self, text):
        """Append text to console output"""
        self.console_text.config(state='normal')
        self.console_text.insert(tk.END, text)
        self.console_text.see(tk.END)
        self.console_text.config(state='disabled')
    
    def clear_console(self):
        """Clear console output"""
        self.console_text.config(state='normal')
        self.console_text.delete(1.0, tk.END)
        self.console_text.config(state='disabled')
    
    def start_download(self):
        """Start the download process"""
        url = self.url_entry.get().strip()
        
        if not url:
            messagebox.showwarning("Input Required", "Please enter a URL to download")
            return
        
        if self.is_downloading:
            messagebox.showwarning("Download in Progress", "A download is already in progress")
            return
        
        self.clear_console()
        self.progress_var.set(0)
        self.status_label.config(text="Starting download...")
        self.details_label.config(text="")
        
        self.download_btn.config(state='disabled')
        self.cancel_btn.config(state='normal')
        self.is_downloading = True
        
        mode = self.download_mode.get()
        thread = threading.Thread(target=self._run_download, args=(url, mode), daemon=True)
        thread.start()
    
    def _run_download(self, url, mode):
        """Run download in background thread"""
        try:
            import sys
            
            # Handle both PyInstaller executable and normal Python script
            if getattr(sys, 'frozen', False):
                # Running as PyInstaller executable
                application_path = sys._MEIPASS
                client_path = os.path.join(application_path, 'client_app')
                idm_path = os.path.join(application_path, 'idm')
                project_root = application_path
            else:
                # Running as normal Python script
                script_dir = os.path.dirname(os.path.abspath(__file__))
                project_root = os.path.abspath(os.path.join(script_dir, '..'))
                client_path = os.path.join(project_root, 'client_app')
                idm_path = os.path.join(project_root, 'idm')

            
            if not os.path.exists(client_path):
                self.append_console(f"ERROR: Client not found at {client_path}\n")
                self.append_console("Please build the project first: ./build_all.sh\n")
                self._download_complete(False)
                return
            
            # Load server settings
            settings = self.load_settings()
            server_ip = settings.get('server_ip', '127.0.0.1')
            server_port = settings.get('server_port', 8080)
            
            # Set environment variables for client
            env = os.environ.copy()
            env['SERVER_IP'] = server_ip
            env['SERVER_PORT'] = str(server_port)
            
            if mode == "smart":
                self.append_console(f"Mode: Smart Download (Server → Internet)\n")
                self.append_console(f"URL: {url}\n")
                self.append_console(f"Target server: {server_ip}:{server_port}\n")
                self.append_console("-" * 60 + "\n\n")
                
                process = subprocess.Popen(
                    [client_path],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    universal_newlines=True,
                    cwd=project_root,
                    env=env
                )
                
                self.current_process = process
                
                try:
                    process.stdin.write("1\n")
                    process.stdin.write(f"{url}\n")
                    process.stdin.flush()
                    process.stdin.close()
                except:
                    pass
                
                for line in process.stdout:
                    if not self.is_downloading:
                        break
                    
                    self.append_console(line)
                    
                    progress_match = re.search(r'(\d+)%', line)
                    if progress_match:
                        percent = int(progress_match.group(1))
                        self.progress_var.set(percent)
                    
                    if "Checking server" in line:
                        self.status_label.config(text="Checking server...")
                    elif "found on server" in line:
                        self.status_label.config(text="Downloading from server...")
                    elif "Downloading from original URL" in line:
                        self.status_label.config(text="Downloading from internet...")
                    elif "Uploading to server" in line:
                        self.status_label.config(text="Uploading to server...")
                    elif "SUCCESS" in line:
                        self.status_label.config(text="Download complete!")
                        self.progress_var.set(100)
                
                process.wait()
                success = (process.returncode == 0)
                
            elif mode == "internet":
                if not os.path.exists(idm_path):
                    self.append_console(f"ERROR: IDM not found at {idm_path}\n")
                    self._download_complete(False)
                    return
                
                self.append_console(f"Mode: Internet Download (Direct)\n")
                self.append_console(f"URL: {url}\n")
                self.append_console("-" * 60 + "\n\n")
                
                process = subprocess.Popen(
                    [idm_path, url],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    universal_newlines=True,
                    cwd=project_root
                )
                
                self.current_process = process
                
                for line in process.stdout:
                    if not self.is_downloading:
                        break
                    
                    self.append_console(line)
                    
                    progress_match = re.search(r'(\d+)%', line)
                    if progress_match:
                        percent = int(progress_match.group(1))
                        self.progress_var.set(percent)
                        self.status_label.config(text=f"Downloading: {percent}%")
                
                process.wait()
                success = (process.returncode == 0)
            
            else:
                self.append_console("Server-only mode not yet implemented\n")
                self.append_console("Use Smart Mode instead\n")
                success = False
            
            self._download_complete(success)
            
        except Exception as e:
            self.append_console(f"\nERROR: {str(e)}\n")
            self._download_complete(False)
    
    def _download_complete(self, success):
        """Called when download completes"""
        self.is_downloading = False
        self.current_process = None
        
        self.after(0, lambda: self.download_btn.config(state='normal'))
        self.after(0, lambda: self.cancel_btn.config(state='disabled'))
        
        if success:
            self.after(0, lambda: self.status_label.config(text="✓ Download completed successfully!"))
            self.after(0, lambda: self.progress_var.set(100))
            self.after(0, lambda: messagebox.showinfo("Success", "Download completed successfully!"))
        else:
            self.after(0, lambda: self.status_label.config(text="✗ Download failed"))
            self.after(0, lambda: messagebox.showerror("Error", "Download failed. Check console for details."))
    
    def load_settings(self):
        """Load settings from settings.json"""
        import json
        settings_path = os.path.join(os.path.dirname(__file__), 'settings.json')
        try:
            if os.path.exists(settings_path):
                with open(settings_path, 'r') as f:
                    return json.load(f)
        except:
            pass
        return {'server_ip': '127.0.0.1', 'server_port': 8080}
    
    def cancel_download(self):
        """Cancel ongoing download"""
        if self.current_process:
            self.current_process.terminate()
            self.append_console("\n\n[CANCELLED BY USER]\n")
        
        self.is_downloading = False
        self.download_btn.config(state='normal')
        self.cancel_btn.config(state='disabled')
        self.status_label.config(text="Download cancelled")
    
    def open_downloads_folder(self):
        """Open the downloads folder"""
        script_dir = os.path.dirname(os.path.abspath(__file__))
        downloads_path = os.path.abspath(os.path.join(script_dir, '..', 'downloads'))
        
        if not os.path.exists(downloads_path):
            os.makedirs(downloads_path)
        
        if os.name == 'nt':
            os.startfile(downloads_path)
        elif os.name == 'posix':
            subprocess.run(['xdg-open', downloads_path])
