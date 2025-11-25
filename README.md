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

---

## Instalação

### Pré-requisitos

Certifique-se de ter os seguintes componentes instalados:

#### Compilador C

Guia de instalação:
- [Configuração do Ambiente C](https://syntaxpathways.com/set-up-c-development-environment/)

#### IDE Code::Blocks

Versão recomendada:
- [Code::Blocks 25.03 Setup](https://sourceforge.net/projects/codeblocks/files/Binaries/25.03/Windows/codeblocks-25.03-setup.exe/download)

#### MySQL Server

Download e instalação:
- [MySQL Community Server](https://dev.mysql.com/downloads/windows/installer/8.0.html)

> **Observação**: Instale todos os programas nos diretórios padrão sugeridos pelos instaladores. Alterações nos caminhos de instalação exigirão configuração manual na IDE.

### Dependências da bilbioteca GTK

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

Após a instalação, abra o terminal **MSYS2 MINGW64** e verifique:

```bash
pkg-config --cflags gtk+-3.0
pkg-config --libs gtk+-3.0
```

Se os comandos retornarem flags, a instalação foi bem-sucedida.

> **Observação**: Caso encontre erros, repita o processo de instalação das dependências e reinicie o terminal.

### Preparação do Banco de Dados

Autentique-se como usuário `root` no **MySQL Command Line Client** e execute o arquivo `init.sql` fornecido no projeto:

```sql
source C:\Users\SeuUsuario\caminho\para\o\projeto\init.sql
```

O script criará automaticamente:
- Banco de dados `pineapple`
- Tabelas `users` e`time_off_requests`
- Usuário padrão `admin` com senha `password`

### Compilação e Execução

1. Abra o arquivo `Overtime Tracker.cbp` com a IDE Code::Blocks;
3. Pressione `F9` para compilar e executar.

---

## Arquitetura do Projeto

### Componentes

#### `main.c`
Lógica de negócio e integração entre interface e banco de dados:
- Gerenciamento do ciclo de vida da aplicação
- Implementação de callbacks e handlers de eventos
- Controle de estado da aplicação
- Validação de regras de negócio
- Gerenciamento de sessões de usuário

#### `database.c`
Camada de abstração para operações de banco de dados:
- Conexão e gerenciamento de chamadas
- Operações CRUD
- Proteção contra SQL Injection

#### `interface.c`
Componentes gráficos reutilizáveis:
- Elementos visuais especializados
- Helpers para manipulação de GTK
- Utilitários de formatação e apresentação

#### `app.glade`
Arquivo da Interface gráfica da aplicação criada apartir do editor Glade

---

## Manual de Utilização

### Painel do Colaborador

#### Dashboard Principal

<div align="center">
<img width="486" height="704" alt="image" src="https://github.com/user-attachments/assets/93590669-1751-4b83-bdc6-6dec380262bd" />
</div>

**Saldo no banco de horas**

Visualização destacada do saldo atual de horas extras com indicador visual da carga horária semanal contratada.

**Lista de Requerimentos**

Histórico completo de todas as solicitações com detalhes de data, quantidade de horas e observações, todas com ordenação cronológica inversa.

#### Criação de Requerimentos

<div align="center">
<img width="521" height="698" alt="image" src="https://github.com/user-attachments/assets/a15ff79d-d599-444e-942d-ea17bee5c8e7" />
</div>

**Formulário de requerimento**

Calendário interativo fácil escolha da data desejada e com validação automática de datas passadas. Controle deslizante de horas Limitado ao saldo disponível e campo de texto para comunicação direta com o gestor.

**Restrições de Horário**
O botão de novo requerimento é automaticamente desabilitado quando o horário atual ultrapassa o expediente ou então o colaborador não possui saldo disponível.


### Painel Admistrativo

#### Dashboard Principal

<div align="center">
<img width="428" height="698" alt="image" src="https://github.com/user-attachments/assets/afacf3bd-c720-40b3-b1ed-bd8f5fa1e261" />
</div>

**Ações rápidas**

Contador de requerimentos pendentes de análise, botão para gerenciamento de colaboradores e análise de requerimentos.

#### Gestão de Colaboradores

<div align="center">
<img width="593" height="703" alt="image" src="https://github.com/user-attachments/assets/87f72b91-98d5-4bb2-9250-16118bae0a80" />
</div>

**Listagem de Colaboradores cadastrados**

Visualização simples de informações como ID e nome de usuário, e ações de adimistração como edição nome, senha e carga horária de usuários, exclusão de dados relacionados e cadastro de novos colaboradores, todos com janelas de confirmação.

#### Gestão de Requerimentos

<div align="center">
<img width="574" height="706" alt="image" src="https://github.com/user-attachments/assets/a7dceb37-0826-465f-a320-121ab08f7e44" />
</div>

**Visualização Detalhada**

Interface dedicada para análise de solicitações, com exibição de todos os dados associados e botões para aprovação ou negação do pedido de horas de folga, com desconto automático da conta do colaborador em caso de deferimento.

---

## Configurações

### Aplicação

O horário limite de abertura de requerimentos, se encontra em `main.c`:

```c
#define SYSTEM_CLOSE_HOUR 18
#define SYSTEM_CLOSE_MINUTE 0
```

### Banco de dados

As credenciais de conexão ao banco de dados se encontram no cabeçalho `database.h`:

```c
#define DATABASE_ADDRESS "localhost"  // Endereço do servidor
#define DATABASE_USER "root"          // Usuário MySQL
#define DATABASE_PASSWORD "123"       // Senha do usuário
#define DATABASE_NAME "pineapple"     // Nome do database
#define DATABASE_PORT 3306            // Porta de conexão
```

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
