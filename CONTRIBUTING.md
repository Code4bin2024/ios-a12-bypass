# Contributing to iOS A12+ Bypass Toolkit

Thank you for your interest in contributing! This document provides guidelines for contributing to this project.

## 📋 Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help create a positive community

## 🚀 How to Contribute

### Reporting Bugs

1. **Check existing issues** to avoid duplicates
2. **Use the bug report template** when creating new issues
3. **Provide detailed information**:
   - Device model and iOS version
   - Steps to reproduce
   - Expected vs actual behavior
   - Error messages and logs (use `--verbose`)

### Suggesting Features

1. **Open an issue** with tag `enhancement`
2. **Describe the feature** clearly
3. **Explain the use case** and benefits

### Pull Requests

1. **Fork the repository**
2. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes**:
   - Follow the existing code style
   - Add tests for new functionality
   - Update documentation as needed
4. **Test thoroughly**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ctest
   ```
5. **Commit with clear messages**:
   ```bash
   git commit -m "feat: add support for new device model"
   ```
6. **Push and create PR**:
   ```bash
   git push origin feature/your-feature-name
   ```

## 💻 Development Guidelines

### Code Style

- **C++ Standard**: C++20
- **Naming Conventions**:
  - Classes: `PascalCase`
  - Functions: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Member variables: `snake_case_` (trailing underscore)
- **Comments**: Use `///` for documentation comments
- **Headers**: Use `#pragma once`

### Project Structure

- `include/iCloudBypassA12/`: Public headers
- `src/`: Implementation files
- `tests/`: Unit tests
- `resources/`: Device plists and data

### Testing

- Write tests for new features using Catch2
- Ensure all tests pass before submitting PR
- Aim for high code coverage

### Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add new feature
fix: bug fix
docs: documentation changes
style: formatting changes
refactor: code refactoring
test: add tests
chore: maintenance tasks
```

## 🔍 Review Process

1. Maintainers will review your PR
2. Address any requested changes
3. Once approved, your PR will be merged

## 📝 Adding Device Support

To add support for a new device:

1. Extract `MobileGestalt.plist` from the device
2. Add to `resources/plists/[DeviceModel]/`
3. Update supported devices list in README
4. Test thoroughly

## 📞 Questions?

- Open an issue with tag `question`
- Check existing discussions

---

Thank you for contributing! 🎉
