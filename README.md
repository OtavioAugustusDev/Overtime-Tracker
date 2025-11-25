# Sistema de Gerenciamento de Banco de Horas

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![GTK](https://img.shields.io/badge/GTK-3.24-purple.svg)
![MySQL](https://img.shields.io/badge/MySQL-8.0-orange.svg)
![C](https://img.shields.io/badge/C-99-lightgrey.svg)

*Solução completa para gestão de horas extras e folgas compensatórias, desenvolvida em C com interface gráfica GTK e banco de dados MySQL*

<img width="1365" alt="Interface do Sistema" src="https://github.com/user-attachments/assets/631b17be-2e11-442d-adfa-e6d71b8112d8" />

[Instalação](#instalação) •
[Uso](#manual-de-uso) •
[Arquitetura](#arquitetura-do-projeto) •
[Segurança](#segurança) •
[Referências](#referências)

</div>

---

## Sobre o projeto

**Overtime Tracker** é um Sistema de Gerenciamento de Banco de Horas de plataforma Desktop feito para atender a um Projeto Integrador Universitário.

### Visão Geral

#### Painel para Colaboradores
- **Dashboard interativo** com sisualização em tempo real do saldo de horas disponíveis
- **Gestão de requerimentos** fluído com um formulário simples para solicitação de folgas compensatórias
- **Validação inteligente**  que previne solicitaçõe inválidas
- **Histórico completo** e detalhado de todos os formulários enviados
- **Restrição de horário** que impede envio de novas solicitações fora de horário comercial

#### Painel para Gestores
- **Painel Administrativo** com visão centralizada de todas as solicitações pendentes
- **Gestão de Usuários** com CRUD completo para gerenciamento de colaboradores
- **Aprovação de requerimentos** de forma tranparente e simplificada
- **Atualizações periódicas** dos dados do sistema
- **Controles de Acesso** com níveis de permissão bem-definidos

### Tecnologias Empregadas

- **Linguagem**: C (C99)
- **Interface Gráfica**: GTK 3.24
- **Banco de Dados**: MySQL 8.0
- **Editor de Interface**: Glade
- **IDE**: Code::Blocks 25.03
- **Compilador**: GCC via MSYS2

---

## Instalação

### Pré-requisitos

Certifique-se de ter os seguintes componentes instalados:

#### 1. Compilador C (GCC)

Guia de instalação:
- [Configuração do Ambiente C](https://syntaxpathways.com/set-up-c-development-environment/)

#### 2. IDE Code::Blocks 25.03

Versão recomendada:
- [Code::Blocks 25.03 Setup](https://sourceforge.net/projects/codeblocks/files/Binaries/25.03/Windows/codeblocks-25.03-setup.exe/download)

#### 3. MySQL Server 8.0

Download e instalação:
- [MySQL Community Server](https://dev.mysql.com/downloads/windows/installer/8.0.html)

> **Importante**: Instale todos os programas nos diretórios padrão sugeridos pelos instaladores. Alterações nos caminhos de instalação exigirão configuração manual na IDE.

### Configuração do Ambiente

#### Instalação das Dependências GTK

Abra o terminal **MSYS2 MSYS** e execute os seguintes comandos:

```bash
# Atualizar o sistema
pacman -Syu

# Instalar ferramentas de compilação
pacman -S mingw-w64-x86_64-toolchain
pacman -S base-devel

# Instalar GTK 3 e ferramentas relacionadas
pacman -S mingw-w64-x86_64-gtk3
pacman -S mingw-w64-x86_64-glade

# Atualização final
pacman -Syu
```

#### Verificação da Instalação

Após a instalação, abra o terminal **MSYS2 MINGW64** e verifique:

```bash
pkg-config --cflags gtk+-3.0
pkg-config --libs gtk+-3.0
```

Se os comandos retornarem flags de compilação, a instalação foi bem-sucedida.

> **Solução de Problemas**: Caso encontre erros, repita o processo de instalação das dependências e reinicie o terminal.

### Configuração do Banco de Dados

#### 1. Criação do Esquema

Abra o **MySQL Command Line Client** e autentique-se com as credenciais do usuário `root`

#### 2. Execução do Script de Inicialização

Execute o arquivo `init.sql` fornecido no projeto:

```sql
source C:\Users\SeuUsuario\caminho\para\o\projeto\init.sql
```

O script criará automaticamente:
- Database `pineapple`
- Tabela `users` com estrutura completa
- Tabela `time_off_requests` com relacionamentos
- Índices otimizados para performance
- Dois usuários padrão para testes

### Compilação do Projeto

1. Abra o arquivo `Overtime Tracker.cbp` no Code::Blocks
3. Pressione `F9` para compilar e executar

---

## Arquitetura do Projeto

### Árvore de Diretórios

```
overtime-tracker/
├── src/
│   ├── main.c              # Orquestração da aplicação
│   ├── database.c          # Camada de acesso ao banco de dados
│   └── interface.c         # Componentes de UI
├── include/                # Cabeçalhos
├── glade/
│   └── app.glade           # Arquivo de interface GTK
├── sql/
│   └── init.sql            # Script de inicialização do banco de dados
├── Overtime Tracker.cbp    # Projeto do Code::Blocks
```

### Componentes Principais

#### main.c
Responsável pela lógica de negócio e integração entre interface e banco de dados:
- Gerenciamento do ciclo de vida da aplicação
- Implementação de callbacks e handlers de eventos
- Controle de estado da aplicação
- Validação de regras de negócio
- Gerenciamento de sessões de usuário

#### database.c
Camada de abstração para operações de banco de dados:
- Conexão e gerenciamento de pool
- Operações CRUD otimizadas
- Proteção contra SQL Injection via prepared statements
- Transações e rollback automático
- Tratamento robusto de erros

#### interface.c
Componentes e widgets customizados:
- Elementos visuais especializados
- Helpers para manipulação de GTK
- Utilitários de formatação e apresentação

#### app.glade
Interface declarativa criada com Glade:
- Layouts responsivos
- Hierarquia de widgets bem estruturada
- Separação clara entre apresentação e lógica
- Facilita manutenção e internacionalização

### Fluxograma

```
[Interface GTK] ←→ [main.c] ←→ [database.c] ←→ [MySQL Server]
       ↑              ↑              ↑
   Eventos      Lógica de      Persistência
   do Usuário   Negócio        de Dados
```

---

## Manual de Uso

### Credenciais Padrão

Após executar o script `init.sql`, o sistema disponibiliza uma conta `admin` nível GESTOR com senha `password`

### Colaborador

#### Dashboard Principal

Ao fazer login como colaborador, você terá acesso a:

**Informações do Saldo**
- Visualização destacada do saldo atual de horas extras
- Indicador visual da carga horária semanal contratada
- Atualização automática após cada operação

**Lista de Requerimentos**
- Histórico completo de todas as solicitações
- Status atualizado (Pendente, Aprovado, Negado)
- Detalhes de data, quantidade de horas e observações
- Ordenação cronológica inversa (mais recentes primeiro)

#### Criação de Requerimentos

Acesse através do botão **"Novo requerimento"** no dashboard:

**1. Seleção de Data**
- Calendário interativo para escolha da data desejada
- Destaque visual da data selecionada
- Validação automática de datas passadas

**2. Definição de Horas**
- Controle deslizante limitado ao saldo disponível
- Indicador numérico em tempo real
- Prevenção de valores inválidos

**3. Observações (Opcional)**
- Campo de texto para comunicação com o gestor
- Suporte a múltiplas linhas
- Contexto adicional sobre a solicitação

**4. Validações Automáticas**
O sistema verifica automaticamente:
- Disponibilidade de saldo suficiente
- Horário de funcionamento do sistema
- Completude dos dados obrigatórios
- Validade da data selecionada (apenas datas futuras são permitidas)
- Prevenção de seleção de datas passadas ou do dia atual

**Restrições de Horário**
O botão de novo requerimento é automaticamente desabilitado quando:
- O horário atual ultrapassa 18:00
- O colaborador não possui saldo disponível
- Mensagens informativas substituem o botão padrão

#### Estados do Botão de Requerimento

| Estado | Mensagem | Disponibilidade |
|--------|----------|-----------------|
| Normal | "Novo requerimento" | Habilitado |
| Fora de horário | "Sistema fechado" | Desabilitado |
| Sem saldo | "Sem saldo disponível" | Desabilitado |

### Gestor

#### Dashboard Administrativo

Interface especializada para gestores com:

**Indicadores**
- Contador de requerimentos pendentes de análise
- Estatísticas em tempo real
- Acesso rápido às principais funcionalidades

**Ações Disponíveis**
- Gerenciamento de colaboradores
- Análise de requerimentos
- Configurações do sistema

#### Gestão de Colaboradores

Painel completo de administração de usuários:

**Listar Colaboradores**
- Visualização de todos os usuários cadastrados
- Informações de ID e nome de usuário
- Ações rápidas de edição e exclusão

**Criar Novo Colaborador**
Campos obrigatórios:
- Nome de usuário (único no sistema)
- Senha de acesso
- Nível de acesso (Colaborador ou Gestor)
- Carga horária semanal

**Editar Colaborador Existente**
- Atualização de todos os dados cadastrais
- Alteração de nível de acesso
- Redefinição de senha
- Ajuste de carga horária

**Excluir Colaborador**
- Confirmação obrigatória antes da exclusão
- Remoção em cascata de requerimentos associados
- Operação irreversível

#### Gestão de Requerimentos

Interface dedicada para análise de solicitações:

**Visualização Detalhada**
Cada requerimento exibe:
- Nome do colaborador solicitante
- Data da folga solicitada
- Quantidade de horas requisitadas
- Observações adicionais
- Status atual da solicitação

**Ações de Aprovação**

Para requerimentos pendentes, o gestor pode:

**Aprovar Requerimento**
- Confirmação via diálogo de segurança
- Dedução automática do saldo do colaborador
- Atualização do status para "Aprovado"
- Notificação visual de sucesso

**Negar Requerimento**
- Confirmação via diálogo de segurança
- Manutenção do saldo do colaborador
- Atualização do status para "Negado"
- Registro permanente da decisão

**Atualização de Saldo**
Quando aprovado, o sistema:
1. Deduz as horas do saldo do colaborador
2. Garante que o saldo não fique negativo
3. Atualiza o status do requerimento
4. Registra timestamp da operação

---

## Segurança

### Proteção Contra SQL Injection

O sistema implementa múltiplas camadas de proteção:

**Escape de Strings**
```c
char escaped_username[256];
mysql_real_escape_string(socket, escaped_username, username, strlen(username));
```

**Validação de Entrada**
- Sanitização de todos os dados do usuário
- Verificação de tipos e tamanhos
- Rejeição de caracteres especiais perigosos

**Prepared Statements**
- Separação entre lógica e dados
- Prevenção de injeção maliciosa
- Performance otimizada

### Controle de Acesso

**Hierarquia de Permissões**
- Colaboradores: Acesso limitado às próprias informações
- Gestores: Acesso completo a funcionalidades administrativas

**Autenticação**
- Senhas armazenadas de forma segura
- Validação de credenciais no servidor
- Sessões gerenciadas em memória

### Validações de dadps

**Integridade de Dados**
- Verificação de saldo antes de aprovações
- Prevenção de valores negativos
- Garantia de consistência transacional
- Bloqueio de datas passadas ou do dia atual em requerimentos
- Limite de horas no slider baseado no saldo disponível do colaborador
---

## Configurações

### Ajuste de Horário do Sistema

O horário limite de abertura de requerimentos, se encontra em `main.c`:

```c
#define SYSTEM_CLOSE_HOUR 18
#define SYSTEM_CLOSE_MINUTE 0
```

### Configuração de Conexão MySQL

Ajuste as credenciais em `database.h` conforme seu ambiente:

```c
#define DATABASE_ADDRESS "localhost"  // Endereço do servidor
#define DATABASE_USER "root"          // Usuário MySQL
#define DATABASE_PASSWORD "123"       // Senha do usuário
#define DATABASE_NAME "pineapple"     // Nome do database
#define DATABASE_PORT 3306            // Porta de conexão
```

## Solução de problemas

### Erro de Conexão MySQL

**Sintoma**: "Erro ao conectar: Can't connect to MySQL server"

**Soluções**:
1. Verifique se o serviço MySQL está rodando
2. Confirme as credenciais em `database.h`
3. Teste a conexão via MySQL Command Line
4. Verifique permissões do usuário MySQL

### Erro de Compilação GTK

**Sintoma**: "gtk/gtk.h: No such file or directory"

**Soluções**:
1. Reinstale as dependências GTK via MSYS2
2. Verifique a variável PATH do sistema
3. Execute `pkg-config --cflags gtk+-3.0` para testar
4. Reinicie a IDE após alterações

### Interface Não Carrega

**Sintoma**: "Erro ao carregar interface: Duplicate object ID"

**Solução**:
Verifique se há IDs duplicados no arquivo `app.glade`. Cada objeto precisa ter um ID único.

### Erros de Linkagem MySQL

**Sintoma**: "undefined reference to mysql_*"

**Solução**:
Adicione `-lmysqlclient` nas opções de linkagem do compilador.

---

## Referências

### Documentação Oficial

- [Documentação GTK 3](https://docs.gtk.org/gtk3/)
- [MySQL C API Reference](https://dev.mysql.com/doc/c-api/8.0/en/)
- [GLib Reference Manual](https://docs.gtk.org/glib/)

### Tutoriais e Guias

**Configuração de Ambiente**
- [Instalação do GNU Compiler Collection](https://syntaxpathways.com/set-up-c-development-environment/)
- [Configuração do GTK no Code::Blocks](https://www.treinaweb.com.br/blog/criando-interfaces-graficas-no-c-com-gtk/)

**Banco de Dados**
- [MySQL Connector/C em aplicações C](https://www.treinaweb.com.br/blog/utilizando-o-mysql-em-uma-aplicacao-c/)
- [Primeiros Passos com MySQL](https://www.treinaweb.com.br/blog/primeiros-passos-com-mysql)

**Linguagem C**
- [Conversão de Tipos em C](https://www.geeksforgeeks.org/c/c-typecasting/)
- [Estruturas em C](https://www.geeksforgeeks.org/c/structures-c/)

**Segurança**
- [Proteção contra SQL Injection](https://dev.mysql.com/doc/c-api/8.0/en/mysql-real-escape-string.html)
- [Gerenciamento de Memória em GLib](https://docs.gtk.org/glib/memory.html)

**Desenvolvimento GTK**
- [Criando Aplicações GTK no Code::Blocks](https://www.treinaweb.com.br/blog/criando-uma-aplicacao-c-com-gtk-no-codeblocks/)
- [Glade Interface Designer](https://glade.gnome.org/)
