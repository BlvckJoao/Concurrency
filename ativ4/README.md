# Sistema de Monitoramento IoT com Sockets TCP e UDP

**Disciplina:** Programação Concorrente (LPII)  
**Aluno:** João Pedro — Matrícula: 6669  

---

## Compilação

```bash
make all        # compila servidor e cliente
make server     # só o servidor
make client     # só o cliente
make clean      # remove binários
```

## Execução

### Terminal 1 — Servidor
```bash
./sensor_server 9000
```

### Terminal 2 — Cliente
```bash
./sensor_client 127.0.0.1 9000 9001
```

> Argumentos: `<ip_servidor> <porta_tcp> <porta_udp_local>`

---

## Comandos disponíveis no cliente

| Comando | Descrição |
|---------|-----------|
| `START /sensor/temperatura` | Inicia fluxo UDP de temperatura |
| `START /sensor/umidade` | Inicia fluxo UDP de umidade |
| `START /sensor/pressao` | Inicia fluxo UDP de pressão |
| `STOP /sensor/<tipo>` | Para o fluxo do sensor |
| `STATUS /sensors` | Lista todos os sensores e status |
| `STATUS /sensor/<tipo>` | Detalhes de um sensor específico |
| `stats` | Exibe estatísticas locais acumuladas |
| `quit` | Envia EXIT ao servidor e encerra |

---

## Exemplo de sessão

```
Conectado ao servidor 127.0.0.1:9000
Socket UDP escutando na porta 9001
Digite comandos:
> START /sensor/temperatura
Resposta: 200 OK | Sensor: temperatura | Status: streaming | ...
[UDP] SEQ:1 | temperatura | 24.73 C | TS:1700000001
[UDP] SEQ:2 | temperatura | 24.81 C | TS:1700000001
> START /sensor/umidade
Resposta: 200 OK | Sensor: umidade | Status: streaming | ...
[UDP] SEQ:3 | temperatura | 24.90 C | TS:1700000002
[UDP] SEQ:1 | umidade | 63.40 % | TS:1700000002
> STATUS /sensors
Resposta: 200 OK | Count: 3 | temperatura: streaming (seq=48) | umidade: streaming (seq=22) | pressao: inactive
> stats
=== Estatisticas Locais ===
temperatura: recebidos=47, perdidos=0 (0.0%), min=23.10, max=26.02, media=24.81
umidade: recebidos=21, perdidos=0 (0.0%), min=61.20, max=65.44, media=63.11
> STOP /sensor/temperatura
Resposta: 200 OK | Sensor: temperatura | Status: stopped
> quit
Enviando EXIT ao servidor...
Conexao encerrada.
```

---

## Portas padrão

| Canal | Porta |
|-------|-------|
| TCP (controle) | 9000 |
| UDP (dados) | 9001 |