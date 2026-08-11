# WSL2 Docker 容器使用 Windows 代理指南

网络结构：`Docker 容器 → WSL2 Ubuntu (172.17.0.1) → Win11 Clash (7897)`

## 前提（只需配置一次）

### 1. Windows 侧：Clash 开放局域网访问

- Clash Verge 打开「局域网连接」（Allow LAN）
- 确认混合端口为 `7897`，在 PowerShell 中验证：

  ```powershell
  netstat -ano | findstr 7897
  ```

  应显示 `0.0.0.0:7897 ... LISTENING`（若只有 `127.0.0.1:7897`，说明局域网连接未生效，重新打开开关或重启 Clash）

- 防火墙放行该端口（管理员 PowerShell）：

  ```powershell
  New-NetFirewallRule -DisplayName "Clash 7897" -Direction Inbound -Protocol TCP -LocalPort 7897 -Action Allow
  ```

  > 注意：如果之前 Clash 弹防火墙授权框时点了「取消」，会留下 Block 规则，需先删除：
  >
  > ```powershell
  > Get-NetFirewallRule | Where-Object {$_.DisplayName -match "clash|verge|mihomo" -and $_.Action -eq "Block"} | Remove-NetFirewallRule
  > ```

### 2. WSL2 Ubuntu：socat 自动转发（只做一次）

往 WSL2 Ubuntu 的 `~/.bashrc` 末尾添加：

```bash
pgrep -f "socat TCP-LISTEN:17897" >/dev/null || nohup socat TCP-LISTEN:17897,fork,reuseaddr,bind=172.17.0.1 TCP:127.0.0.1:7897 >/dev/null 2>&1 &
```

原理：WSL2 使用 mirrored（镜像）网络模式，`127.0.0.1:7897` 即是 Windows 的 Clash；socat 将其转发到 WSL 的 docker 网桥地址 `172.17.0.1:17897`，使容器可达。

如未安装 socat：`sudo apt install -y socat`

## 每次使用（容器内）

### 3. 设置代理环境变量

容器的 `~/.bashrc` 已配置自动检测代理；手动设置的话：

```bash
export http_proxy=http://172.17.0.1:17897
export https_proxy=http://172.17.0.1:17897
```

### 4. 验证连通性

```bash
curl -sI --max-time 8 https://www.google.com/generate_204   # 返回 204 即通
```

## 排错速查

| 现象 | 原因 | 解决 |
|---|---|---|
| 连 `172.17.0.1:17897` 报 `Connection refused` | WSL 里 socat 没运行 | 重进一次 WSL，或手动执行第 2 步的 socat 命令 |
| WSL 里 `curl -x http://127.0.0.1:7897 -I https://www.google.com/generate_204` 都不通 | Clash 没开 / 未允许局域网 / 防火墙拦截 | 检查第 1 步 |
| apt 不走代理 | apt 不读 `http_proxy` 环境变量 | 加参数：`-o Acquire::http::Proxy=http://172.17.0.1:17897 -o Acquire::https::Proxy=http://172.17.0.1:17897` |
| `172.17.0.1` 地址变了 | docker 网桥重建 | 容器里 `cat /proc/net/route` 查网关地址，替换所有配置中的该地址 |
| `host.docker.internal` 解析到错误地址 | DNS 缓存了过期 IP | 不要使用该域名，直接用 `172.17.0.1` |

## 一句话记忆

**Clash 开端口 → WSL 里 socat 做桥 → 容器里 export 代理指向 `172.17.0.1:17897`**
