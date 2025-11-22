# PowerShell script to setup GitHub repository on Windows

Write-Host "╔════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║       GitHub Repository Setup Helper                   ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""

# Check if git is installed
if (!(Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Host "❌ Error: git is not installed" -ForegroundColor Red
    Write-Host ""
    Write-Host "Install git first:"
    Write-Host "  Download from https://git-scm.com/"
    exit 1
}

Write-Host "✓ Git is installed: $(git --version)" -ForegroundColor Green
Write-Host ""

# Get GitHub repository URL
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host "Step 1: Create a new repository on GitHub" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""
Write-Host "1. Go to: https://github.com/new"
Write-Host "2. Repository name: ddos-detection-mpi (or your choice)"
Write-Host "3. Description: Distributed DDoS Detection System using MPI"
Write-Host "4. Choose: Public or Private"
Write-Host "5. Do NOT initialize with README (we have one!)"
Write-Host "6. Click 'Create repository'"
Write-Host ""
Read-Host "Press ENTER after creating the repository"
Write-Host ""

$REPO_URL = Read-Host "Enter your GitHub repository URL (e.g., https://github.com/username/ddos-detection-mpi.git)"

if ([string]::IsNullOrWhiteSpace($REPO_URL)) {
    Write-Host "❌ Error: Repository URL cannot be empty" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Repository URL: $REPO_URL"
Write-Host ""

# Initialize git repository
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host "Step 2: Initialize Git Repository" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""

# Check if already a git repository
if (Test-Path ".git") {
    Write-Host "⚠️  Git repository already exists" -ForegroundColor Yellow
    $REINIT = Read-Host "Do you want to reinitialize? (y/N)"
    if ($REINIT -eq "y" -or $REINIT -eq "Y") {
        Remove-Item -Recurse -Force .git
        git init
        Write-Host "✓ Repository reinitialized" -ForegroundColor Green
    } else {
        Write-Host "✓ Using existing repository" -ForegroundColor Green
    }
} else {
    git init
    Write-Host "✓ Git repository initialized" -ForegroundColor Green
}

Write-Host ""

# Configure git user (if not set)
$gitUser = git config user.name
if ([string]::IsNullOrWhiteSpace($gitUser)) {
    Write-Host "Git user not configured. Let's set it up:"
    $GIT_NAME = Read-Host "Enter your name"
    $GIT_EMAIL = Read-Host "Enter your email"
    git config user.name "$GIT_NAME"
    git config user.email "$GIT_EMAIL"
    Write-Host "✓ Git user configured" -ForegroundColor Green
}

Write-Host ""
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host "Step 3: Add Files to Repository" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""

# Add all files
git add .
Write-Host "✓ Files staged for commit" -ForegroundColor Green

# Show what will be committed
Write-Host ""
Write-Host "Files to be committed:"
git status --short
Write-Host ""

# Create initial commit
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host "Step 4: Create Initial Commit" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""

$commitMessage = @"
Initial commit: DDoS Detection System using MPI

- MPI-based distributed detection architecture
- Entropy, PCA, and CUSUM detection algorithms
- Optional CUDA GPU acceleration
- Dynamic blocklist management
- Comprehensive metrics and reporting
- FlowSpec and ACL rule generation
- Complete documentation and examples
"@

git commit -m $commitMessage

Write-Host "✓ Initial commit created" -ForegroundColor Green
Write-Host ""

# Add remote
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host "Step 5: Add Remote Repository" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""

# Remove existing remote if exists
git remote remove origin 2>$null

git remote add origin "$REPO_URL"
Write-Host "✓ Remote 'origin' added: $REPO_URL" -ForegroundColor Green
Write-Host ""

# Push to GitHub
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host "Step 6: Push to GitHub" -ForegroundColor Yellow
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Yellow
Write-Host ""

# Determine default branch name
$BRANCH = git rev-parse --abbrev-ref HEAD
Write-Host "Current branch: $BRANCH"
Write-Host ""

# Push to GitHub
Write-Host "Pushing to GitHub..."
git push -u origin $BRANCH

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "╔════════════════════════════════════════════════════════╗" -ForegroundColor Green
    Write-Host "║            🎉 SUCCESS! Repository on GitHub!          ║" -ForegroundColor Green
    Write-Host "╚════════════════════════════════════════════════════════╝" -ForegroundColor Green
    Write-Host ""
    $repoWebURL = $REPO_URL -replace '\.git$', ''
    Write-Host "Your repository is now live at:"
    Write-Host "🔗 $repoWebURL" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Next steps:"
    Write-Host "  1. Visit your repository on GitHub"
    Write-Host "  2. Add a description and topics"
    Write-Host "  3. Enable GitHub Actions (if desired)"
    Write-Host "  4. Share with your team!"
    Write-Host ""
    Write-Host "To make future changes:"
    Write-Host "  git add <files>"
    Write-Host "  git commit -m 'Your commit message'"
    Write-Host "  git push"
    Write-Host ""
} else {
    Write-Host ""
    Write-Host "❌ Error: Failed to push to GitHub" -ForegroundColor Red
    Write-Host ""
    Write-Host "Common solutions:"
    Write-Host "  1. Check your GitHub credentials"
    Write-Host "  2. Make sure you have push access"
    Write-Host "  3. Try: git push -u origin $BRANCH --force (if needed)"
    Write-Host "  4. Check repository URL is correct"
    Write-Host ""
}
