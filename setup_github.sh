#!/bin/bash
# GitHub Repository Setup Script

echo "╔════════════════════════════════════════════════════════╗"
echo "║       GitHub Repository Setup Helper                   ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# Check if git is installed
if ! command -v git &> /dev/null; then
    echo "❌ Error: git is not installed"
    echo ""
    echo "Install git first:"
    echo "  Ubuntu/WSL: sudo apt-get install git"
    echo "  Windows: Download from https://git-scm.com/"
    exit 1
fi

echo "✓ Git is installed: $(git --version)"
echo ""

# Get GitHub repository URL
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 1: Create a new repository on GitHub"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1. Go to: https://github.com/new"
echo "2. Repository name: ddos-detection-mpi (or your choice)"
echo "3. Description: Distributed DDoS Detection System using MPI"
echo "4. Choose: Public or Private"
echo "5. Do NOT initialize with README (we have one!)"
echo "6. Click 'Create repository'"
echo ""
read -p "Press ENTER after creating the repository..."
echo ""

read -p "Enter your GitHub repository URL (e.g., https://github.com/username/ddos-detection-mpi.git): " REPO_URL

if [ -z "$REPO_URL" ]; then
    echo "❌ Error: Repository URL cannot be empty"
    exit 1
fi

echo ""
echo "Repository URL: $REPO_URL"
echo ""

# Initialize git repository
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 2: Initialize Git Repository"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check if already a git repository
if [ -d ".git" ]; then
    echo "⚠️  Git repository already exists"
    read -p "Do you want to reinitialize? (y/N): " REINIT
    if [ "$REINIT" = "y" ] || [ "$REINIT" = "Y" ]; then
        rm -rf .git
        git init
        echo "✓ Repository reinitialized"
    else
        echo "✓ Using existing repository"
    fi
else
    git init
    echo "✓ Git repository initialized"
fi

echo ""

# Configure git user (if not set)
if [ -z "$(git config user.name)" ]; then
    echo "Git user not configured. Let's set it up:"
    read -p "Enter your name: " GIT_NAME
    read -p "Enter your email: " GIT_EMAIL
    git config user.name "$GIT_NAME"
    git config user.email "$GIT_EMAIL"
    echo "✓ Git user configured"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 3: Add Files to Repository"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Add all files
git add .
echo "✓ Files staged for commit"

# Show what will be committed
echo ""
echo "Files to be committed:"
git status --short
echo ""

# Create initial commit
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 4: Create Initial Commit"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

git commit -m "Initial commit: DDoS Detection System using MPI

- MPI-based distributed detection architecture
- Entropy, PCA, and CUSUM detection algorithms
- Optional CUDA GPU acceleration
- Dynamic blocklist management
- Comprehensive metrics and reporting
- FlowSpec and ACL rule generation
- Complete documentation and examples"

echo "✓ Initial commit created"
echo ""

# Add remote
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 5: Add Remote Repository"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Remove existing remote if exists
git remote remove origin 2>/dev/null

git remote add origin "$REPO_URL"
echo "✓ Remote 'origin' added: $REPO_URL"
echo ""

# Push to GitHub
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 6: Push to GitHub"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Determine default branch name
BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "Current branch: $BRANCH"
echo ""

# Push to GitHub
echo "Pushing to GitHub..."
git push -u origin "$BRANCH"

if [ $? -eq 0 ]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════╗"
    echo "║            🎉 SUCCESS! Repository on GitHub!          ║"
    echo "╚════════════════════════════════════════════════════════╝"
    echo ""
    echo "Your repository is now live at:"
    echo "🔗 ${REPO_URL%.git}"
    echo ""
    echo "Next steps:"
    echo "  1. Visit your repository on GitHub"
    echo "  2. Add a description and topics"
    echo "  3. Enable GitHub Actions (if desired)"
    echo "  4. Share with your team!"
    echo ""
    echo "To make future changes:"
    echo "  git add <files>"
    echo "  git commit -m 'Your commit message'"
    echo "  git push"
    echo ""
else
    echo ""
    echo "❌ Error: Failed to push to GitHub"
    echo ""
    echo "Common solutions:"
    echo "  1. Check your GitHub credentials"
    echo "  2. Make sure you have push access"
    echo "  3. Try: git push -u origin $BRANCH --force (if needed)"
    echo "  4. Check repository URL is correct"
    echo ""
fi
