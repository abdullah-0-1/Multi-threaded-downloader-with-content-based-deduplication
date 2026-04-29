#!/bin/bash

# =========================================
# DSA Server - Maintenance Menu
# =========================================

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

show_menu() {
    clear
    echo -e "${BLUE}=========================================${NC}"
    echo -e "${BLUE}🔧 DSA Server Maintenance${NC}"
    echo -e "${BLUE}=========================================${NC}"
    echo ""
    echo "1.  📊 Check server status"
    echo "2.  📜 View server logs"
    echo "3.  🔄 Restart server"
    echo "4.  🛑 Stop server"
    echo "5.  ▶️  Start server (background)"
    echo "6.  🗑️  Clear server storage"
    echo "7.  💾 Check storage usage"
    echo "8.  📁 List stored files"
    echo "9.  🔨 Rebuild server"
    echo "10. 💻 System information"
    echo "11. 📡 Network status ${RED}[REMOTE ONLY]${NC}"
    echo "12. 🔍 Show running processes"
    echo "0.  🚪 Exit"
    echo ""
    echo -e "${BLUE}=========================================${NC}"
    echo -n "Select option: "
}

check_status() {
    echo -e "\n${YELLOW}📊 Checking server status...${NC}\n"
    
    if pgrep -f "./server_app" > /dev/null; then
        echo -e "${GREEN}✅ Server is RUNNING${NC}"
        ps aux | grep "./server_app" | grep -v grep
        echo ""
        
        # Check if screen session exists
        if screen -ls | grep -q "dsa-server"; then
            echo -e "${GREEN}✓ Screen session: dsa-server (Active)${NC}"
        fi
        
        # Check port 8080
        if netstat -tuln 2>/dev/null | grep -q ":8080"; then
            echo -e "${GREEN}✓ Port 8080: Listening${NC}"
        elif ss -tuln 2>/dev/null | grep -q ":8080"; then
            echo -e "${GREEN}✓ Port 8080: Listening${NC}"
        fi
    else
        echo -e "${RED}❌ Server is NOT running${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

view_logs() {
    echo -e "\n${YELLOW}📜 Viewing server logs...${NC}"
    echo -e "${YELLOW}(Press Ctrl+A then D to detach)${NC}\n"
    sleep 2
    
    if screen -ls | grep -q "dsa-server"; then
        screen -r dsa-server
    else
        echo -e "${RED}❌ No screen session found${NC}"
        echo "Server might not be running in background"
    fi
    
    read -p "Press Enter to continue..."
}

restart_server() {
    echo -e "\n${YELLOW}🔄 Restarting server...${NC}\n"
    
    # Stop existing server
    if pgrep -f "./server_app" > /dev/null; then
        echo "Stopping existing server..."
        screen -S dsa-server -X quit 2>/dev/null
        pkill -f "./server_app" 2>/dev/null
        sleep 2
    fi
    
    # Start new server (from correct directory)
    echo "Starting server in background..."
    screen -dmS dsa-server bash -c 'cd ~/dsa-project/server && ./server_app'
    sleep 2
    
    if pgrep -f "./server_app" > /dev/null; then
        echo -e "${GREEN}✅ Server restarted successfully${NC}"
    else
        echo -e "${RED}❌ Failed to restart server${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

stop_server() {
    echo -e "\n${YELLOW}🛑 Stopping server...${NC}\n"
    
    if pgrep -f "./server_app" > /dev/null; then
        screen -S dsa-server -X quit 2>/dev/null
        pkill -f "./server_app" 2>/dev/null
        sleep 2
        
        if ! pgrep -f "./server_app" > /dev/null; then
            echo -e "${GREEN}✅ Server stopped${NC}"
        else
            echo -e "${RED}❌ Failed to stop server${NC}"
        fi
    else
        echo -e "${YELLOW}Server is not running${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

start_server() {
    echo -e "\n${YELLOW}▶️  Starting server...${NC}\n"
    
    if pgrep -f "./server_app" > /dev/null; then
        echo -e "${YELLOW}⚠️  Server is already running${NC}"
    else
        screen -dmS dsa-server bash -c 'cd ~/dsa-project/server && ./server_app'
        sleep 2
        
        if pgrep -f "./server_app" > /dev/null; then
            echo -e "${GREEN}✅ Server started in background${NC}"
            echo "View logs with: screen -r dsa-server"
        else
            echo -e "${RED}❌ Failed to start server${NC}"
        fi
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

clear_storage() {
    echo -e "\n${RED}⚠️  WARNING: This will delete all stored files!${NC}"
    read -p "Are you sure? (yes/no): " confirm
    
    if [ "$confirm" == "yes" ]; then
        echo -e "\n${YELLOW}🗑️  Clearing server storage...${NC}\n"
        
        rm -rf server-storage/*
        mkdir -p server-storage
        
        echo -e "${GREEN}✅ Storage cleared${NC}"
        echo -e "${YELLOW}ℹ️  Restart server to apply changes${NC}"
    else
        echo -e "${YELLOW}Cancelled${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

check_storage() {
    echo -e "\n${YELLOW}💾 Storage Usage:${NC}\n"
    
    if [ -d "server-storage" ]; then
        du -sh server-storage/
        echo ""
        echo "File count: $(ls server-storage/ 2>/dev/null | wc -l)"
        echo ""
        echo "Top 10 largest files:"
        du -h server-storage/* 2>/dev/null | sort -rh | head -10
    else
        echo -e "${RED}Storage directory not found${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

list_files() {
    echo -e "\n${YELLOW}📁 Stored Files:${NC}\n"
    
    if [ -d "server-storage" ]; then
        ls -lh server-storage/ | tail -n +2
    else
        echo -e "${RED}Storage directory not found${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

rebuild_server() {
    echo -e "\n${YELLOW}🔨 Rebuilding server...${NC}\n"
    
    # Stop server first
    if pgrep -f "./server_app" > /dev/null; then
        echo "Stopping server..."
        screen -S dsa-server -X quit 2>/dev/null
        pkill -f "./server_app" 2>/dev/null
        sleep 2
    fi
    
    # Run build script
    if [ -f "build_server.sh" ]; then
        ./build_server.sh
    else
        echo -e "${RED}❌ build_server.sh not found${NC}"
    fi
    
    echo ""
    read -p "Press Enter to continue..."
}

system_info() {
    echo -e "\n${YELLOW}💻 System Information:${NC}\n"
    
    echo "Hostname: $(hostname)"
    echo "OS: $(lsb_release -d | cut -f2)"
    echo "Kernel: $(uname -r)"
    echo ""
    echo "CPU:"
    lscpu | grep "Model name" | cut -d: -f2 | xargs
    echo "CPUs: $(nproc)"
    echo ""
    echo "Memory:"
    free -h | grep -E "Mem|Swap"
    echo ""
    echo "Disk:"
    df -h ~ | grep -E "Filesystem|/$"
    echo ""
    echo "Uptime: $(uptime -p)"
    
    echo ""
    read -p "Press Enter to continue..."
}

network_status() {
    echo -e "\n${RED}⚠️  [REMOTE ONLY]${NC}"
    echo -e "${YELLOW}📡 Network Status:${NC}\n"
    echo -e "${YELLOW}This command is designed for remote server environments only.${NC}\n"
    
    echo "Public IP: $(curl -s ifconfig.me)"
    echo "Private IP: $(hostname -I | awk '{print $1}')"
    echo ""
    echo "Listening Ports:"
    sudo netstat -tuln 2>/dev/null | grep LISTEN || sudo ss -tuln | grep LISTEN
    echo ""
    echo "Active Connections:"
    netstat -tn 2>/dev/null | grep ":8080" || ss -tn | grep ":8080"
    
    echo ""
    read -p "Press Enter to continue..."
}

show_processes() {
    echo -e "\n${YELLOW}🔍 Running Processes:${NC}\n"
    
    ps aux | head -1
    ps aux | grep -E "server_app|screen" | grep -v grep
    
    echo ""
    read -p "Press Enter to continue..."
}

# Main loop
while true; do
    show_menu
    read option
    
    case $option in
        1) check_status ;;
        2) view_logs ;;
        3) restart_server ;;
        4) stop_server ;;
        5) start_server ;;
        6) clear_storage ;;
        7) check_storage ;;
        8) list_files ;;
        9) rebuild_server ;;
        10) system_info ;;
        11) network_status ;;
        12) show_processes ;;
        0) 
            echo -e "\n${GREEN}Goodbye!${NC}\n"
            exit 0
            ;;
        *)
            echo -e "\n${RED}Invalid option${NC}"
            sleep 1
            ;;
    esac
done