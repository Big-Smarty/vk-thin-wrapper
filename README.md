<h3 align="center">vk-thin-wrapper</h3>

  <p align="center">
    A simple to use vulkan thin wrapper library (headeronly)
    <br />
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

vk-thin-wrapper is a simple headeronly vulkan thin raii wrapper library I've been working on for a while now.
It might be slow.
It might even crash your entire pc.
But at least it isnt made in rust.


<!-- GETTING STARTED -->
## Getting Started

### Prerequisites

This is an example of how to list things you need to use the software and how to install them.
* Dependencies:
  - volk
  - spdlog
  - clang/gcc with c++20 support

<!-- USAGE EXAMPLES -->
## Usage

Simple example:

```c++
using Instance_T = bs::thin_wrappers::thin_wrapper<VkInstance, VkInstanceCreateInfo, no_parent_t, 
thin_wrappers::vkTrampFn<&vkCreateInstance>, thin_wrappers::vkTrampFn<&vkDestroyInstance>>;
```

<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request



<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.



<!-- CONTACT -->
## Contact

Discord: Big-Smarty#2123
EMail: budaniasco@gmail.com
