# Task Management System

This directory contains atomic tasks for the HY300 projector porting project.

## Structure

- `docs/tasks/` - Active tasks numbered sequentially
- `docs/tasks/completed/` - Completed tasks archive

## Task Format

Each task file should follow this format:

```markdown
# Task: [Brief Description]

**Status:** [pending|in_progress|completed]
**Priority:** [high|medium|low]
**Assigned:** [AI Agent]
**Created:** [Date]
**Context:** [Reference to ai/contexts/ files]

## Description

[Detailed description of the atomic task]

## Prerequisites

- [List of dependencies]

## Acceptance Criteria

- [ ] [Specific, measurable criteria]
- [ ] [More criteria]

## Notes

[Any additional information]
```

## Task Lifecycle

1. **Created** - Task is identified and documented
2. **In Progress** - AI agent is actively working on task
3. **Completed** - Task meets all acceptance criteria, moved to completed/

## Current Task Counter

Next task number: 004