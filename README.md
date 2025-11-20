# Overtime Tracker
Sistema gerenciador de horas extras desenvolvido em C com interface gráfica GTK4 e banco de dados MySQL.

<img width="1365" height="714" alt="image" src="https://github.com/user-attachments/assets/631b17be-2e11-442d-adfa-e6d71b8112d8" />

## Configuração do Ambiente de Desenvolvimento

### Pré-requisitos

1. **Compilador C**
   - [Guia de instalação](https://syntaxpathways.com/set-up-c-development-environment/)

2. **IDE Code::Blocks 25.03**
   - [Download aqui](https://sourceforge.net/projects/codeblocks/files/Binaries/25.03/Windows/codeblocks-25.03-setup.exe/download)

3. **MySQL Server**
   - [Download aqui](https://dev.mysql.com/downloads/windows/installer/8.0.html)

> **Observação**: Instale todos os programas nos diretórios padrão. Caso contrário, será necessário configurar os caminhos manualmente na IDE.

## Instalação das dependências

### Biblioteca GTK4 e utilitários

No terminal **MSYS2 MSYS** execute:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-toolchain
pacman -S base-devel
pacman -S mingw-w64-x86_64-gtk4
pacman -S mingw-w64-x86_64-glade
pacman -Syu
```

Ao final, no terminal **MSYS2 MINGW64** verifique se a instalação foi bem-sucedida executando:

```bash
pkg-config --cflags gtk4
```

> **Observação**: Caso enfrente algum problema, repita os passos de instalação novamente.

## Criação do Banco de Dados

1. Abra o **MySQL Command Line Client**
2. Faça login com a senha do usuário `root`
3. Execute o arquivo:

```sql
source C:\Users\SeuUsuario\caminho\para\o\projeto\init.sql
```

As informações do banco de dados se encontram no cabeçalho `database.h`:

```c
#define DATABASE_ADDRESS "localhost"
#define DATABASE_USER "root"
#define DATABASE_PASSWORD "123"
#define DATABASE_NAME "pineapple"
#define DATABASE_PORT 3306
```

## Compilação e Execução

1. Abra o arquivo `Overtime Tracker.cbp` no Code::Blocks
2. Compile e execute o projeto: `F9`

## Arquivos

```
overtime-tracker/
├── main.c              # Lógica da aplicação e callbacks da interface
├── database.c          # Operações de leitura e escrita no banco de dados
├── interface.c         # Componentes de interface
```

## Manual de uso da aplicação

Ao executar o arquivo `init.sql`, o SGBD do MySQL criará dois usuários padrão:

| Usuário | Senha | Nível |
|---------|-------|------|
| otavio  | 1234  | GESTOR |
| breno   | 1234  | USER |

### Operações nível colaborador

Ao fazer login em uma conta nível COLABORADOR, você terá acesso as seguintes telas:

#### Dashboard
- Visualização do saldo atual no banco de horas
- Informações sobre a carga horária semanal
- Lista dos requerimentos feitos (atualizada a cada 2 segundos)

#### Criação de requerimento
1. Acessível apartir do botão "Novo requerimento"
2. Dispõe de um calendário para seleção da data desejada para a folga
3. Controle deslizante para ajuste da quantidade de horas pretendida
4. Campo para escrita de notas para o gestor, de caráter opcional
5. Os dados inseridos são validados pelo sistema antes de serem enviados ao gestor

> **Validação automática:**
> O envio de novos requerimentos só é permitido dentro do horário de funcionamento e quando há saldo disponível na conta. O sistema impede requerimentos vazios ou com datas passadas além de limitar a quantidade máxima de horas ao saldo disponível na conta do colaborador.

### Operações nível gestor

Ao fazer login em uma conta nível GESTOR, você terá acesso as seguintes telas:

#### Dashboard do Gestor
- Contador de requerimentos pendentes de análise
- Acesso às opções administrativas de gerência de usuários

#### Gerência de colaboradores
Permite o cadastro de novos colaboradores, edição e exclusão de usuários já cadastrados.

#### Gerência de requerimentos
Permite a visualização completa dos dados dos requerimentos recebidos e as opções para aprovação ou rejeição dos pedidos.

> **Observação**:
> O saldo é automaticamente reduzido da conta do colaborador quando o requerimento é aprovado. O sistema também possui um horário limite para abertura de novos requerimentos, a propriedade pode ser consultada no arquivo principal `main.c`:
> ```c
> #define SYSTEM_LOCK_TIME 999
> ```

## Funcionalidades implementadas
- Autenticação de usuário
- Interface gráfica
- Envio de requerimentos
- Painel administrativo
- Sistema próprio de gestão de usuários
- Proteção contra SQL Injection

## Referências

- [Instalação do GNU COMPILER COLLECTION](https://syntaxpathways.com/set-up-c-development-environment/)
- [Instalação e configuração da biblioteca GTK4 na IDE Code::Blocks](https://www.treinaweb.com.br/blog/criando-interfaces-graficas-no-c-com-gtk/)
- [Instalação e configuração do connector MySQL na IDE Code::Blocks](https://www.treinaweb.com.br/blog/utilizando-o-mysql-em-uma-aplicacao-c/)
- [Instalação das dependências de compilação do GTK4](https://www.treinaweb.com.br/blog/criando-uma-aplicacao-c-com-gtk-no-codeblocks/)
- [Documentação da biblioteca GTK4](https://docs.gtk.org/gtk4/)
- [Utilização da linha do comando do Cliente MySQL](https://www.treinaweb.com.br/blog/primeiros-passos-com-mysql)
- [Conversão de tipos em C](https://www.geeksforgeeks.org/c/c-typecasting/)
- [Definição de estruturas em C](https://www.geeksforgeeks.org/c/structures-c/)
- [Proteção contra SQL Injection](https://dev.mysql.com/doc/c-api/8.0/en/mysql-real-escape-string.html)
- [Gerenciamento de memória em GLib](https://docs.gtk.org/glib/memory.html)
