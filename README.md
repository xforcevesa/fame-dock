# Fame Dock: An efficient and open-source database engine

## Synopsis

Fame Dock is an efficient and open-source database engine that provides a high-level abstraction of the underlying storage system. It is designed to be easy to use and easy to integrate into existing applications. Fame Dock is written in C++ and is designed to be portable across multiple platforms. Fame Dock is designed to be scalable and can handle large datasets with ease. Fame Dock is designed to be easy to maintain and extend, and it provides a rich set of features that make it a versatile and powerful database engine. The following is a high-level overview of Fame Dock:

![Fame Dock Architecture](architecture.png)

## Motivation

Fame Dock was created to provide a high-level abstraction of the underlying storage system, making it easy to use and easy to integrate into existing applications. Fame Dock is designed to be scalable and can handle large datasets with ease. Fame Dock is designed to be easy to maintain and extend, and it provides a rich set of features that make it a versatile and powerful database engine.

## Features

Fame Dock provides the following features:

- **Schema-less design:** Fame Dock is schema-less, meaning that it does not require the user to specify the schema of the data beforehand. Instead, Fame Dock automatically infers the schema from the data and stores it in a schema-like format. This allows for easy data migration and evolution.

- **Efficient storage:** Fame Dock is designed to be efficient and can handle large datasets with ease. It uses a combination of indexing, compression, and caching techniques to achieve high performance.

- **Rich set of features:** Fame Dock provides a rich set of features that SQL languages also provide, which make it a versatile and powerful database engine. These features include:

  - **Transactions:** Fame Dock supports transactions, which allow multiple operations to be executed atomically.

  - **Query language:** Fame Dock provides a powerful query language that allows users to retrieve data from the database using a variety of criteria.

  - **Joins:** Fame Dock supports joins, which allow users to retrieve data from multiple tables based on their relationships.

  - **Views:** Fame Dock supports views, which allow users to create virtual tables based on the results of a query.

  - **Indexes:** Fame Dock supports indexes, which allow users to quickly locate data based on specific criteria.

  - **Functions:** Fame Dock supports functions, which allow users to perform complex operations on data.

  - **Triggers:** Fame Dock supports triggers, which allow users to execute custom code when certain events occur.

  - **Security:** Fame Dock provides security features, including encryption, access control, and authentication.

- **Open-source:** Fame Dock is open-source, which means that it is freely available for anyone to use, modify, and distribute. Fame Dock is released under the BSD license, which allows for both personal and commercial use.

- **Portable:** Fame Dock is designed to be portable across multiple platforms, including Linux, Windows, and macOS. Fame Dock uses a combination of C++ and platform-specific libraries to achieve this portability.

- **Scalable:** Fame Dock is designed to be scalable and can handle large datasets with ease. It uses a combination of indexing, caching, and compression techniques to achieve high performance. Fame Dock also provides features such as sharding and replication to allow for horizontal scaling.

## Getting Started

To get started with Fame Dock, you can follow the steps below:

1. Install Fame Dock on your system. You can download the latest release source code from the [GitHub repository](https://github.com/xforcevesa/fame-dock) and build Fame Dock from source using the provided instructions:

- To build Fame Dock server:

   ```bash
   git clone https://github.com/xforcevesa/fame-dock.git
   cd rmdb
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

- To build Fame Dock client:

   ```bash
   cd rmdb/rmdb_client
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
   ```

2. Start the Fame Dock server:

- To start the Fame Dock server:

   ```bash
   cd rmdb/build/bin
   ./rmdb_server
   ```

3. Connect to the Fame Dock server using a client:

- To connect to the Fame Dock server using the Fame Dock client:

   ```bash
   cd rmdb/rmdb_client/build/
   ./rmdb_client
   ```

## Documentation

Fame Dock provides extensive documentation, including a user guide and a reference manual. The user guide provides a step-by-step guide to using Fame Dock, while the reference manual provides a complete reference of all the available features and commands. The documentation is available in both HTML and PDF formats.

## License

Fame Dock is released under the MIT license, which allows for both personal and commercial use.
