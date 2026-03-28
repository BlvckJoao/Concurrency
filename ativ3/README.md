Projeto de: João Pedro Pereira Maranhão

Matrícula: 20240036669

Cenário: A — Restaurante Concorrente

Data: 26/03/2026

---

## 0- Estrutura do Projeto e Compilação

O projeto foi organizado da seguinte forma:

```
ativ3/
├── include/
│   └── restaurant/
│       ├── queue.h
│       └── restaurante.h
├── src/
│   ├── queue.c
│   └── restaurante.c
├── README.md
└── restaurante.exe
```

Para compilar, basta executar na raiz do projeto:

```bash
gcc src/*.c -Iinclude -o restaurante -lpthread
```

O flag `-Iinclude` garante que o compilador encontre os headers em `include/restaurant/`, e `-lpthread` linka a biblioteca POSIX de threads necessária para `pthread_*` e `sem_*`.

## 1 Identificação

**Nome:** João Pedro Pereira Maranhão

**Matrícula:** 20240036669

**Cenário escolhido:** A — Restaurante Concorrente

**Cálculo dos parâmetros (M = 6669):**

| Parâmetro | Fórmula | Cálculo | Resultado |
| --- | --- | --- | --- |
| NUM_CHEFS | (M % 4) + 2 | (6669 % 4) + 2 = 1 + 2 | **3** |
| NUM_STOVES | (M % 3) + 2 | (6669 % 3) + 2 = 1 + 2 | **3** |
| NUM_TABLES | ((M / 10) % 5) + 5 | ((666) % 5) + 5 = 1 + 5 | **6** |
| NUM_WAITERS | ((M / 100) % 3) + 2 | ((66) % 3) + 2 = 0 + 2 | **2** |
| PREP_TIME | ((M / 100) % 3) + 1 | ((66) % 3) + 1 = 0 + 1 | **1s** |

---

## 2 Decisões de Projeto

### Arquitetura geral

A solução foi organizada em torno de um **monitor global** (`RestaurantMonitor`) que encapsula todos os mecanismos de sincronização e o estado compartilhado do sistema. Essa escolha centraliza a responsabilidade de acesso concorrente e facilita a inicialização e destruição ordenada dos recursos.

O fluxo de dados funciona como um **pipeline de três estágios**: cliente → garçom → cozinheiro. Cada estágio se comunica com o próximo por meio de uma fila compartilhada protegida por mutex e variáveis de condição, implementando o padrão produtor-consumidor em dois níveis.

### Mecanismos de sincronização utilizados

**1. Semáforo contador (`sem_t sem_stoves`) — acesso aos fogões**

O semáforo foi a escolha natural para controlar o recurso limitado dos fogões, pois permite que até `NUM_STOVES` cozinheiros os utilizem simultaneamente sem necessidade de acordar threads desnecessariamente. A alternativa seria um mutex + contador manual, que funcionaria mas adicionaria complexidade sem benefício real — o semáforo já implementa exatamente essa semântica.

**2. Mutex + variáveis de condição — filas produtor-consumidor**

Foram utilizadas duas filas independentes com seus próprios mutexes e pares de condicionais:

- `client_queue` (cliente → garçom): o cliente deposita o pedido e o garçom consome.
- `order_queue` (garçom → cozinheiro): o garçom repassa e o cozinheiro consome.

O uso de duas condicionais por fila (`has_item` e `has_space`) permite que produtores e consumidores bloqueiem apenas quando necessário, evitando busy-waiting. A alternativa de usar `sleep()` em loop seria inaceitável em termos de desperdício de CPU.

**3. Mutex + variável de condição por mesa — sincronização cliente/cozinheiro**

Cada mesa possui seu próprio mutex e `food_ready` condicional. O cliente dorme esperando `table->ready == 1`, que é sinalizado pelo cozinheiro após finalizar o prato. Essa abordagem de granularidade fina (um mutex por mesa, não um global) evita que a entrega de um prato bloqueie outras mesas desnecessariamente.

**4. Mutex de estatísticas (`stats_mutex`)**

Separado do mutex da fila deliberadamente. Se usássemos o mesmo mutex da `order_queue` para proteger `invoicing` e `total_orders`, um cozinheiro registrando estatísticas bloquearia um garçom tentando inserir um pedido — contenção desnecessária. Mutexes separados com responsabilidades bem definidas reduzem o tempo de lock.

**5. Barreira (`pthread_barrier_t`) — sincronização de rodadas**

Garçons e cozinheiros aguardam na barreira ao final do expediente, garantindo que nenhuma thread de serviço encerre antes das demais. A barreira foi inicializada com `NUM_WAITERS + NUM_CHEFS` participantes. Clientes não participam da barreira pois têm ciclo de vida independente — cada um termina ao pagar sua conta.

### Principais desafios

O maior desafio foi definir a **condição de encerramento** das threads de serviço. Garçons e cozinheiros ficam em loop infinito esperando itens nas filas. A solução foi a flag `restaurant_open`: quando o `main` detecta que todos os clientes terminaram, seta `restaurant_open = 0` e dispara `pthread_cond_broadcast` nas duas filas, fazendo com que as threads em espera acordem, verifiquem a condição de saída e encerrem. Sem o broadcast, essas threads ficariam bloqueadas para sempre após o último cliente.

---

## 3 Resultados Experimentais

### Log de execução

```
Restaurante aberto! Temos 3 cozinheiros, 2 garçons e 6 mesas.
[Cliente 1] aguardando atendimento na mesa 5
[Cozinheiro 1] pronto para cozinhar!
[Cozinheiro 2] pronto para cozinhar!
[Cozinheiro 3] pronto para cozinhar!
[Garçom 1] pronto para atender!
[Garçom 1] pegou pedido mesa 5
[Garçom 1] pedido da mesa 5 inserido na fila
[Cozinheiro 1] preparando prato da mesa 5
[Garçom 2] pronto para atender!
[Cozinheiro 1] prato pronto para a mesa 5
[Cliente 1] recebeu pedido e está comendo (tempo de espera: 1.00 segundos)
[Cliente 1] terminou de comer e vai embora
[Cliente 2] aguardando atendimento na mesa 5
[Garçom 1] pegou pedido mesa 5
[Garçom 1] pedido da mesa 5 inserido na fila
[Cozinheiro 2] preparando prato da mesa 5
[Cozinheiro 2] prato pronto para a mesa 5
[Cliente 2] recebeu pedido e está comendo (tempo de espera: 4.00 segundos)
[Cliente 2] terminou de comer e vai embora
[Cliente 3] aguardando atendimento na mesa 5
[Garçom 2] pegou pedido mesa 5
[Garçom 2] pedido da mesa 5 inserido na fila
[Cozinheiro 3] preparando prato da mesa 5
[Cozinheiro 3] prato pronto para a mesa 5
[Cliente 3] recebeu pedido e está comendo (tempo de espera: 7.00 segundos)
[Cliente 3] terminou de comer e vai embora
[Cliente 4] aguardando atendimento na mesa 5
[Garçom 1] pegou pedido mesa 5
[Garçom 1] pedido da mesa 5 inserido na fila
[Cozinheiro 1] preparando prato da mesa 5
[Cozinheiro 1] prato pronto para a mesa 5
[Cliente 4] recebeu pedido e está comendo (tempo de espera: 10.00 segundos)
[Cliente 4] terminou de comer e vai embora
[Cliente 5] aguardando atendimento na mesa 5
[Garçom 2] pegou pedido mesa 5
[Garçom 2] pedido da mesa 5 inserido na fila
[Cozinheiro 2] preparando prato da mesa 5
[Cozinheiro 2] prato pronto para a mesa 5
[Cliente 5] recebeu pedido e está comendo (tempo de espera: 13.00 segundos)
[Cliente 5] terminou de comer e vai embora
[Cliente 6] aguardando atendimento na mesa 5
[Garçom 1] pegou pedido mesa 5
[Garçom 1] pedido da mesa 5 inserido na fila
[Cozinheiro 3] preparando prato da mesa 5
[Cozinheiro 3] prato pronto para a mesa 5
[Cliente 6] recebeu pedido e está comendo (tempo de espera: 16.00 segundos)
[Cliente 6] terminou de comer e vai embora
[Garçom 2] terminou de atender e vai embora (tempo de trabalho: 18.00 segundos)
[Garçom 1] terminou de atender e vai embora (tempo de trabalho: 18.00 segundos)
[Cozinheiro 3] terminou de cozinhar e vai embora (tempo de trabalho: 18.00 segundos)
[Cozinheiro 1] terminou de cozinhar e vai embora (tempo de trabalho: 18.00 segundos)
[Cozinheiro 2] terminou de cozinhar e vai embora (tempo de trabalho: 18.00 segundos)
Restaurante fechado! Faturamento total: R$170.00
```

### Estatísticas coletadas

| Métrica | Valor |
| --- | --- |
| Total de clientes atendidos | 6 |
| Faturamento total | R$ 170,00 |
| Tempo médio de espera por cliente | ~9,5 segundos |
| Tempo total de execução | ~18 segundos |
| Pedidos por segundo | ~0,33 pedidos/s |
| Tempo de preparo por prato (PREP_TIME) | 1 segundo |

### Análise dos resultados

O sistema se comportou conforme o esperado em termos de correção: todos os clientes foram atendidos sem deadlocks, nenhuma condição de corrida foi detectada e o faturamento final de R$170,00 corresponde à soma dos preços dos 6 pedidos. 

Contudo, o log revela um comportamento sequencial não esperado: todos os clientes utilizaram exclusivamente a mesa 5. Isso ocorre porque o mapeamento `table_id = client_id % NUM_TABLES` distribui os IDs de 1 a 6 em tabelas de 1 a 5, mas o cliente aguarda a mesa ficar livre — e como os clientes chegam em sequência e a mesa 5 sempre é liberada antes do próximo entrar, o sistema acaba serializando o uso das mesas em vez de paralelizá-lo. Para corrigir, seria necessário que os clientes tentassem qualquer mesa livre ao invés de uma fixa por ID.

O tempo de espera crescendo linearmente (1s, 4s, 7s...) confirma que os clientes estão sendo atendidos em série. Com o mapeamento de mesas corrigido, múltiplos clientes poderiam ocupar mesas simultaneamente e ser atendidos em paralelo pelos 3 cozinheiros e 2 garçons, reduzindo o tempo total e aumentando os pedidos por segundo.

---

## 6.4 Reflexão sobre Uso de IA

**Ferramentas utilizadas:** Claude (Anthropic)

**Para quais tarefas:** O Claude foi usado como ferramenta de apoio ao longo de todo o desenvolvimento, principalmente para: discutir a arquitetura inicial do monitor e identificar problemas de design antes de escrever o código; revisar e corrigir bugs específicos (a atribuição faltante da fila no `monitor_initialize`, a inicialização errada do semáforo com `NUM_WAITERS` ao invés de `NUM_STOVES`, os nomes semanticamente invertidos das condicionais); e entender a interação correta entre `pthread_cond_wait`, broadcast e a flag de encerramento.

**A IA cometeu erros?** Sim. Nas sugestões iniciais, os nomes `is_empty` e `is_full` foram herdados do esboço original com a semântica invertida — a IA apontou o problema, mas o código de exemplo em uma das respostas ainda usava nomes que geravam confusão, o que me fez questionar antes de adotar. A identificação veio da leitura cuidadosa do código em conjunto com a explicação.

**O que aprendi que não saberia sem a IA:** A distinção prática entre usar um semáforo para recurso limitado (fogões) versus mutex+condição para fila (onde preciso tanto de exclusão mútua quanto de notificação de estado) ficou muito mais clara através da discussão do que teria ficado só com a leitura do material da disciplina. Além disso, o uso de 2 variáveis condicionais para situações diferentes as quais o uso de uma apenas causava deadlock.

**O que aprendi apesar da IA:** A IA inicialmente sugeriu uma barreira incluindo os clientes junto com garçons e cozinheiros. Só entendi por que isso causaria deadlock ao raciocinar sobre os ciclos de vida distintos de cada tipo de thread — clientes terminam individualmente, enquanto garçons e cozinheiros ficam em loop. Esse entendimento veio de trabalhar com o código, não de aceitar a sugestão passivamente.

**Como validei o entendimento:** Reescrevi cada função de sincronização sem consultar as sugestões, depois comparei com o que havia discutido para verificar se os `wait`/`signal` correspondiam corretamente às condições de guarda. Também tracei manualmente o fluxo de execução para os casos de fila cheia e fila vazia para confirmar que não havia possibilidade de deadlock nas seções críticas.


## 6.5 Ponderações finais sobre o projeto

Foi um projeto divertido e desafiador de se trabalhar e aprendi bastante sobre criação de sistemas concorrentes e resolução de problemas de sincronização, o que contribui muito para o entendimento de como uma boa arquitetura pode tornar sua solução mais elegante e estável e como sistemas concorrentes reais se comportam.