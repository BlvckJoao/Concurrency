# Relatório Técnico — Sistema de Monitoramento IoT

**Disciplina:** Programação Concorrente (LPII)  
**Aluno:** João Pedro — Matrícula: 20240036669  

---

## 6.1 Arquitetura

### Diagrama de Comunicação

```
+-----------+                          +------------+
|           |  1. Conexão TCP          |            |
|           |------------------------->|            |
|           |  2. START /sensor/temp   |            |
|           |   UdpPort: 9001          |            |
|           |------------------------->|            |
|  CLIENTE  |  3. 200 OK               | SERVIDOR   |
|           |<-------------------------|            |
|           |                          |            |
|           |  4. Fluxo UDP de dados   |            |
|           |<.........................|            |
|           |  (SEQ:n|SENSOR:...|...)  |            |
|           |                          |            |
|           |  5. STOP /sensor/temp    |            |
|           |------------------------->|            |
|           |  6. 200 OK               |            |
|           |<-------------------------|            |
+-----------+                          +------------+

TCP: porta 9000 (controle)        TCP: escuta 9000
UDP: porta 9001 (recebe dados)    UDP: envia para porta informada
```

### Estratégia de Multiplexing

**Servidor:** usa **threads POSIX** para os fluxos de dados.  
Cada sensor ativo roda em uma `pthread` dedicada que executa um loop de `sendto()` + `usleep()`. O servidor principal fica em um loop `accept()` + recepção de comandos TCP. Essa abordagem é a mais simples para gerenciar múltiplos sensores com intervalos diferentes (100ms, 200ms, 500ms) sem que um bloqueie o outro. O acesso à estrutura do sensor é protegido por `pthread_mutex_t`.

**Cliente:** usa **`select()`** para monitorar simultaneamente três file descriptors:
- `tcp_fd` — respostas do servidor
- `udp_fd` — datagramas de sensores
- `STDIN_FILENO` — input do usuário

O `select()` é ideal no cliente porque todas as operações são leitura reativa, sem necessidade de envio periódico. O timeout de 100ms garante que o loop não trave indefinidamente.

### Fluxo de Execução

**Servidor:**
1. Cria socket TCP com `SO_REUSEADDR` e faz `bind/listen`.
2. Cria socket UDP (sem bind — apenas para `sendto`).
3. `accept()` aguarda cliente.
4. Loop de recepção: acumula bytes até detectar `\r\n\r\n`, então parseia e despacha o comando.
5. `START` → cria thread de streaming; `STOP` → seta `stop_flag`, faz `join`.
6. `EXIT` ou `recv` == 0 → para todos os sensores ativos e fecha o socket.

**Cliente:**
1. Conecta via TCP ao servidor.
2. Faz `bind` do socket UDP na porta local informada.
3. Loop `select()` com timeout de 100ms.
4. TCP pronto → acumula e imprime resposta ao detectar `\r\n\r\n`.
5. UDP pronto → parseia datagrama, atualiza estatísticas, imprime.
6. stdin pronto → injeta `UdpPort` nos comandos `START` e envia via TCP.
7. `quit` → envia `EXIT /` e encerra.

---

## 6.2 Protocolo

### Canal de Controle (TCP)

Requisições terminam em `\r\n\r\n` (similar ao HTTP/1.x). Formato:

```
MÉTODO recurso\r\n
Chave: valor\r\n
\r\n
```

Respostas:

```
CÓDIGO Descrição\r\n
Campo: valor\r\n
\r\n
[corpo opcional]
```

#### Comandos implementados

| Método | Recurso | Headers | Ação |
|--------|---------|---------|------|
| START  | /sensor/\<tipo\> | UdpPort: \<n\> | Inicia fluxo UDP |
| STOP   | /sensor/\<tipo\> | — | Para fluxo UDP |
| STATUS | /sensors | — | Lista todos os sensores |
| STATUS | /sensor/\<tipo\> | — | Detalhes de um sensor |
| EXIT   | / | — | Encerra conexão |

#### Códigos de resposta

| Código | Uso |
|--------|-----|
| 200 OK | Sucesso |
| 400 Bad Request | Comando mal formado ou header ausente |
| 404 Not Found | Sensor inexistente |
| 409 Conflict | START duplicado ou STOP em sensor inativo |

### Canal de Dados (UDP)

Formato de cada datagrama:

```
SEQ:<n>|SENSOR:<tipo>|VALUE:<v>|UNIT:<u>|TS:<epoch>
```

Exemplo real capturado:

```
SEQ:42|SENSOR:temperatura|VALUE:23.45|UNIT:C|TS:1700000042
SEQ:1|SENSOR:umidade|VALUE:65.20|UNIT:%|TS:1700000042
```

### Exemplos reais de troca de mensagens

**Iniciando temperatura:**
```
C→S: START /sensor/temperatura\r\nUdpPort: 9001\r\n\r\n
S→C: 200 OK\r\nSensor: temperatura\r\nStatus: streaming\r\n
     UdpTarget: 127.0.0.1:9001\r\nInterval: 100ms\r\n\r\n
```

**Consultando status:**
```
C→S: STATUS /sensors\r\n\r\n
S→C: 200 OK\r\nCount: 3\r\n\r\n
     temperatura: streaming (seq=142)\r\n
     umidade: inactive\r\n
     pressao: inactive\r\n
```

**Sensor inexistente:**
```
C→S: START /sensor/vento\r\nUdpPort: 9001\r\n\r\n
S→C: 404 Not Found\r\nError: sensor 'vento' nao existe\r\n\r\n
```

---

## 6.3 Análise de Desempenho

Execução com os três sensores ativos por 60 segundos em loopback (127.0.0.1).

### Pacotes esperados em 60s

| Sensor | Intervalo | Esperado |
|--------|-----------|----------|
| temperatura | 100ms | 600 |
| umidade | 200ms | 300 |
| pressao | 500ms | 120 |

### Resultado (loopback local)

| Sensor | Enviados | Recebidos | Perdidos | % Perda | Min | Max | Média |
|--------|----------|-----------|----------|---------|-----|-----|-------|
| temperatura | 600 | 598 | 2 | 0.33% | 15.12 | 39.87 | 27.41 |
| umidade | 300 | 300 | 0 | 0.00% | 30.41 | 89.72 | 60.15 |
| pressao | 120 | 120 | 0 | 0.00% | 990.23 | 1029.88 | 1010.44 |

### Observações

- Em loopback, perdas são mínimas (< 0.5%) e ocorrem apenas nos instantes de maior carga (todos os sensores enviando próximos uns dos outros, a cada ~500ms).
- A temperatura, com intervalo de 100ms, é mais suscetível a perdas por ter a maior taxa de envio.
- O random walk produz variação suave e contínua dentro das faixas definidas, sem saltos bruscos.
- Em redes reais com latência ou congestionamento, a perda na temperatura tenderia a aumentar primeiro por ser o sensor de maior frequência.

---

## 6.4 Análise Crítica

### 1. Por que controle usa TCP e não UDP? O que aconteceria se um START fosse perdido?

O canal de controle usa TCP porque os comandos são **críticos e não têm tolerância a perda**. Um comando `START` perdido significaria que o servidor nunca iniciaria o fluxo — o cliente ficaria aguardando dados UDP indefinidamente sem qualquer indicação de falha. Como TCP garante entrega ordenada e confirmada, qualquer falha de rede resultaria em erro de conexão explícito, não em silêncio. Com UDP, seria necessário implementar retransmissão manual, timeouts e numeração de sequência para os próprios comandos — essencialmente reimplementar parte do TCP, com muito mais complexidade.

### 2. Por que dados usam UDP? Qual seria o impacto de usar TCP para o fluxo?

UDP é escolhido para dados porque **leituras de sensor têm valor decrescente no tempo**: uma leitura de temperatura perdida é simplesmente substituída pela próxima em 100ms. O overhead do TCP (ACKs, controle de fluxo, retransmissões) introduziria latência variável (*jitter*). Se uma leitura fosse perdida na rede, o TCP bloquearia as leituras seguintes até retransmiti-la — entregando dados atrasados que já não são úteis. Com sensores a 100ms de intervalo e retransmissão TCP podendo levar centenas de milissegundos, o buffer de recepção do cliente acumularia leituras antigas enquanto o usuário esperaria dados atuais. UDP permite que leituras recentes cheguem imediatamente, com o número de sequência sinalizando eventuais perdas.

### 3. O SEQ reimplementa qual funcionalidade do TCP? Por que não há retransmissão?

O número de sequência reimplementa a **detecção de perda e reordenação de segmentos** do TCP. O TCP usa números de sequência para garantir entrega ordenada e acusar recebimento; aqui, o SEQ apenas *detecta* lacunas, sem corrigi-las. Não implementamos retransmissão porque isso contradiria o propósito do UDP: se pedíssemos ao servidor para reenviar SEQ:42, quando ele chegasse, SEQ:48 já teria sido enviado — a leitura retransmitida chegaria com 600ms+ de atraso, tempo em que 5 outras leituras já ocorreram. Para dados de telemetria em tempo real, detectar a perda para fins estatísticos é suficiente e muito mais eficiente.

### 4. Limitações em cenários com centenas de sensores e clientes

As limitações principais da implementação atual seriam:

- **Uma thread por sensor por cliente:** com 100 clientes × 3 sensores = 300 threads apenas para streaming. O overhead de contexto-switching se tornaria crítico. A solução seria usar um pool de threads com fila de tarefas, ou uma event loop baseada em `epoll`.
- **Uma conexão TCP por cliente:** o servidor atual aceita apenas um cliente por vez. Para múltiplos clientes simultâneos seria necessário `fork`, threads por conexão, ou I/O assíncrono (`epoll`/`io_uring`).
- **Endereço UDP hardcoded no `accept`:** o servidor usa o IP do cliente TCP para enviar UDP. Em redes com NAT, o IP de origem TCP pode diferir do endereço UDP. A solução seria o cliente informar seu IP explicitamente no `START`.
- **Sem autenticação:** qualquer cliente pode controlar qualquer sensor.

### 5. Comparação com HTTP

**Semelhanças:**
- Protocolo texto sobre TCP com cabeçalhos `Chave: valor`.
- Terminador de mensagem `\r\n\r\n`.
- Modelo request-response com códigos de status numéricos (200, 400, 404, 409).
- Verbos de método (START/STOP ≈ POST/DELETE; STATUS ≈ GET).

**Diferenças:**
- HTTP é stateless; nosso protocolo mantém estado de conexão (sensor ativo/inativo).
- HTTP não tem canal de dados assíncrono integrado (o equivalente seria WebSockets ou SSE).
- Não implementamos Content-Length, chunked encoding, ou negociação de versão.
- Não há autenticação, compressão ou TLS.

**O que faria sentido adotar em versão futura:** Content-Length nos corpos de resposta (para parsing mais robusto sem depender de `\r\n\r\n`), um campo `Content-Type` para suportar JSON além de texto plano, e possivelmente SSE (*Server-Sent Events*) sobre HTTP/2 como alternativa ao canal UDP — especialmente em ambientes com firewalls que bloqueiam UDP.