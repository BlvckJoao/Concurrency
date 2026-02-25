RELATÓRIO – EXERCÍCIO PRÁTICO 002 (extra)

Disciplina: LPII – Programação Concorrente – 2025.2
Aluno: João Pedro Maranhão
Matrícula: (PREENCHER AQUI)

1. Quantos mutexes foram utilizados e por quê?

Foi utilizado um mutex global para proteger os recursos compartilhados da estrutura Sala.

A escolha por um único mutex foi feita para simplificar o controle de concorrência e garantir a correção do programa com menor complexidade. Como o sistema possui apenas 50 assentos e 20 threads, o uso de um único mutex não compromete significativamente o desempenho e reduz o risco de erros de sincronização.

O mutex protege:

A operação de verificação e reserva de assentos

A atualização dos contadores de reservas com sucesso e falha

2. Onde foram colocados lock/unlock e justificativa

O uso de pthread_mutex_lock e pthread_mutex_unlock foi realizado nos seguintes pontos:

a) Dentro da função tentar_reserva()

A verificação se dois assentos estão livres e a marcação deles como ocupados foi protegida pelo mutex.

Justificativa

Essa operação precisa ser atômica. Sem proteção, duas threads poderiam:

Verificar que os mesmos dois assentos estão livres

Reservar simultaneamente

Gerar duplicidade de reserva e inconsistência no estado da sala

O lock foi colocado antes da verificação dos assentos e o unlock após a tentativa (tanto em caso de sucesso quanto de falha).

b) Na atualização dos contadores reservas_sucesso e reservas_falha

A atualização desses contadores também foi protegida pelo mutex.

Justificativa

Incrementos não são operações atômicas. Sem proteção adequada, poderia ocorrer condição de corrida, resultando em:

Contadores inconsistentes

Divergência na verificação final do sistema

3. O que poderia acontecer sem a proteção

Sem o uso de mutex, poderiam ocorrer:

Dois clientes reservando o mesmo par de assentos

Assentos marcados incorretamente

Contadores inconsistentes

Falha na verificação de integridade (assentos ocupados ≠ 2 × clientes atendidos)

Esses problemas caracterizam condições de corrida típicas em sistemas concorrentes.

4. Considerações finais

A solução implementada garante:

Criação correta das 20 threads de clientes

Uso adequado de pthread_create e pthread_join

Identificação e proteção das seções críticas

Consistência final dos dados, validada pela verificação de integridade

A decisão por um mutex global priorizou simplicidade e correção, atendendo plenamente aos requisitos do exercício proposto.

O código foi desenvolvido com auxílio de LLM como ferramenta de apoio para organização estrutural e revisão de boas práticas, mantendo total entendimento da implementação e das decisões de projeto.